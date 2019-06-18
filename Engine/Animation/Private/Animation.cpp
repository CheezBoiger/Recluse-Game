// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Animation.hpp"
#include "Skeleton.hpp"
#include "Clip.hpp"

#include "Core/Exception.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Utility/Time.hpp"

namespace Recluse {

const size_t                        Animation::kMaxAnimationThreadCount   = 2;

Animation& gAnimation()
{
  return Animation::instance();
}


void Animation::onStartUp()
{
}


void Animation::onShutDown()
{
  for (auto& it : m_animObjects) {
    delete it.second;
  }
}


void Animation::updateState(r64 dt)
{
  r32 globalTime = static_cast<r32>(Time::currentTime());
  size_t H = (m_sampleJobs.size() >> 1);
  
  // animate jobs.
  for (auto& job : m_sampleJobs) {
    doSampleJob(job, static_cast<r32>(dt));
  }

  for (auto& job : m_blendJobs) {
    doBlendJob(job, static_cast<r32>(dt));
  }

  m_sampleJobs.clear();
}


AnimHandle* Animation::createAnimHandle(uuid64 id)
{
  auto it = m_animObjects.find(id);
  if (it != m_animObjects.end()) return nullptr;
  
  AnimHandle* pObj = new AnimHandle(id);
  m_animObjects[id] = pObj;
  return pObj;
}


void Animation::freeAnimHandle(AnimHandle* pObj)
{
  if (!pObj) return;

  auto it = m_animObjects.find(pObj->_uuid);
  if (it == m_animObjects.end()) return;

  m_animObjects.erase(it);
  delete pObj;
  pObj = nullptr;
}


void Animation::submitJob(const AnimJobSubmitInfo& info)
{
  switch (info._type) {
    case ANIM_JOB_TYPE_SAMPLE:
    {
      m_sampleJobs.push_back(info);
    } break;
    case ANIM_JOB_TYPE_BLEND:
    {
      
    } break;
    case ANIM_JOB_TYPE_LAYERED_BLEND:
    {
      
    } break;
    default: break;  
  }
}


Matrix4 Animation::linearInterpolate(JointPose* currPose, JointPose* nextPose, r32 currTime, r32 nextTime, r32 t)
{
  Matrix4 resultTransform = Matrix4::identity();
  
  r32 dt = (t - currTime) /  (nextTime - currTime);
  if (isinf(dt)) return resultTransform;

  Vector3 translate = Vector3::lerp(currPose->_trans, nextPose->_trans, dt);
  Vector3 scale = Vector3::lerp(currPose->_scale, nextPose->_scale, dt);
  Quaternion rot = Quaternion::slerp(currPose->_rot, nextPose->_rot, dt);

  Matrix4 mT = Matrix4::translate(Matrix4(), translate);
  Matrix4 mS = Matrix4::scale(Matrix4(), scale);
  Matrix4 mR = rot.toMatrix4();
  resultTransform = mS * mR * mT;

  return resultTransform;
}


void Animation::getCurrentAndNextPoseIdx(u32* outCurr, u32* outNext, AnimClip* pClip, r32 lt)
{
  for (size_t i = 0; i < pClip->_aAnimPoseSamples.size(); ++i) {
    AnimPose& pose = pClip->_aAnimPoseSamples[i];
    if (pose._time > lt) {
      *outCurr = static_cast<u32>(i) - 1u;
      *outNext = static_cast<u32>(i);
      *outCurr = *outCurr > pClip->_aAnimPoseSamples.size() ?
        pClip->_aAnimPoseSamples.size() - 1u : *outCurr;
      break;
    }
  }
}


b32 Animation::emptyPoseSamples(AnimClip* pClip, i32 currPoseIdx, i32 nextPoseIdx)
{
  if (pClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses.empty()
    || pClip->_aAnimPoseSamples[nextPoseIdx]._aLocalPoses.empty()) {
    return true;
  }
  return false;
}


void Animation::applyMorphTargets(AnimHandle* pOutput, AnimClip* pClip, i32 currPoseIdx, i32 nextPoseIdx, r32 lt)
{
  if (!pClip->_aAnimPoseSamples[currPoseIdx]._morphs.empty() &&
    !pClip->_aAnimPoseSamples[nextPoseIdx]._morphs.empty()) {
    auto& currMorphs = pClip->_aAnimPoseSamples[currPoseIdx]._morphs;
    auto& nextMorphs = pClip->_aAnimPoseSamples[nextPoseIdx]._morphs;
    pOutput->_finalMorphs.resize(currMorphs.size());
    for (size_t i = 0; i < currMorphs.size(); ++i) {
      pOutput->_finalMorphs[i] = Lerpf(currMorphs[i], nextMorphs[i], lt);
    }
  }
}


void Animation::doSampleJob(AnimJobSubmitInfo& job, r32 gt)
{
  if (!job._output->_currState._bEnabled) { return; }
  r32 tau = job._output->_currState._tau;
  r32 rate = job._output->_currState._fPlaybackRate;
  r32 lt = job._output->_currState._fCurrLocalTime + gt * rate;
  if (lt > job._pBaseClip->_fDuration) {
    lt -= job._pBaseClip->_fDuration;
    job._output->_currState._tau = gt;
  }
  if (lt < 0.0f) {
    lt = job._pBaseClip->_fDuration + lt;
    if (lt < 0.0f) {
      lt += job._pBaseClip->_fDuration;
      job._output->_currState._tau = gt;
    }
  }
  job._output->_currState._fCurrLocalTime = lt;
  Skeleton* pSkeleton = Skeleton::getSkeleton(job._pBaseClip->_skeletonId);
  Matrix4* palette = job._output->_finalPalette;
  u32 paletteSz = job._output->_paletteSz;

  u32 currPoseIdx = 0;
  u32 nextPoseIdx = 0;

  getCurrentAndNextPoseIdx(&currPoseIdx, &nextPoseIdx, job._pBaseClip, lt);

  applyMorphTargets(job._output, job._pBaseClip, currPoseIdx, nextPoseIdx, lt);

  if (emptyPoseSamples(job._pBaseClip, currPoseIdx, nextPoseIdx)) return;

  if (pSkeleton) {
    doSkeletalAnimation(job, pSkeleton, lt,currPoseIdx, nextPoseIdx);
  } else {
    doMechanicalAnimation(job, lt, currPoseIdx, nextPoseIdx);
  }
}


void Animation::doMechanicalAnimation(AnimJobSubmitInfo& job,
                                      r32 lt,
                                      u32 currPoseIdx,
                                      u32 nextPoseIdx)
{
  AnimPose* currAnimPose = &job._pBaseClip->_aAnimPoseSamples[currPoseIdx];
  AnimPose* nextAnimPose = &job._pBaseClip->_aAnimPoseSamples[nextPoseIdx];
  for (size_t i = 0; i < job._pBaseClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses.size() &&
                      i < job._pBaseClip->_aAnimPoseSamples[nextPoseIdx]._aLocalPoses.size(); ++i) {
    JointPose* currJoint = &currAnimPose->_aLocalPoses[i];
    JointPose* nextJoint = &nextAnimPose->_aLocalPoses[i];
    Matrix4 localTransform = linearInterpolate(currJoint, nextJoint, currAnimPose->_time, nextAnimPose->_time, lt);
    Matrix4 parentTransform = job._output->_finalPalette[currJoint->_id];
    job._output->_finalPalette[currJoint->_id] = localTransform;
  }
}


void Animation::doSkeletalAnimation(AnimJobSubmitInfo& job, 
                                    Skeleton* pSkeleton, 
                                    r32 lt,
                                    u32 currPoseIdx, 
                                    u32 nextPoseIdx)
{
  Matrix4 globalTransform;
  b32 rootInJoints = pSkeleton ? pSkeleton->_rootInJoints : false;
  {
    Matrix4 localTransform = linearInterpolate(
      &job._pBaseClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses[0],
      &job._pBaseClip->_aAnimPoseSamples[nextPoseIdx]._aLocalPoses[0],
      job._pBaseClip->_aAnimPoseSamples[currPoseIdx]._time,
      job._pBaseClip->_aAnimPoseSamples[nextPoseIdx]._time,
      lt);
    if (rootInJoints) {
      job._output->_finalPalette[0] = localTransform;
    }
    globalTransform = localTransform;
  }

  AnimPose* currAnimPose = &job._pBaseClip->_aAnimPoseSamples[currPoseIdx];
  AnimPose* nextAnimPose = &job._pBaseClip->_aAnimPoseSamples[nextPoseIdx];
  // TODO(): Need to figure out how to map nodes to certain indices, as gltf does not 
  // sort animation samples by joint hierarchy.
  for (size_t i = 1; i < job._pBaseClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses.size(); ++i) {
    size_t idx = (rootInJoints ? i : i - 1);
    JointPose* currJoint = &currAnimPose->_aLocalPoses[i];
    JointPose* nextJoint = &nextAnimPose->_aLocalPoses[i];
    Matrix4 localTransform = linearInterpolate(currJoint, nextJoint, currAnimPose->_time, nextAnimPose->_time, lt);

    // For now, we are checking marked node ids of the gltf hierarchy.
    for (size_t j = 0; j < pSkeleton->_joints.size(); ++j) {
      if (pSkeleton->_joints[j]._id == currJoint->_id) {
        job._output->_finalPalette[j] = localTransform;
        break;
      }
    }
  }

  applySkeletonPose(job._output->_finalPalette, globalTransform, pSkeleton);
}


void Animation::applySkeletonPose(Matrix4* pOutput, Matrix4 globalMatrix, Skeleton* pSkeleton)
{
  if (!pSkeleton) return;

  for (size_t i = 0; i < pSkeleton->_joints.size(); ++i) {
    Matrix4 parentTransform;
    u8 parentId = pSkeleton->_joints[i]._iParent;
    if (parentId == Joint::kNoParentId) {
      parentTransform = globalMatrix;
    } else {
      parentTransform = pOutput[parentId];
    }
    pOutput[i] = pOutput[i] * parentTransform;
  }

  for (size_t i = 0; i < pSkeleton->_joints.size(); ++i) {
    pOutput[i] =  pSkeleton->_joints[i]._invBindPose * 
                  pOutput[i] * 
                  pSkeleton->_rootInvTransform;
  }
}


void Animation::doBlendJob(AnimJobSubmitInfo& job, r32 gt)
{
  for (auto& layer : job._layers) {
    
  }
}
} // Recluse