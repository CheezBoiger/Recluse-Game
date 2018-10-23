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
  return Animation::Instance();
}


void Animation::OnStartUp()
{
}


void Animation::OnShutDown()
{
  for (auto& it : m_animObjects) {
    delete it.second;
  }
}


void Animation::UpdateState(r64 dt)
{
  r32 globalTime = static_cast<r32>(Time::CurrentTime());
  size_t H = (m_sampleJobs.size() >> 1);
  
  // animate jobs.
  for (auto& job : m_sampleJobs) {
    DoSampleJob(job, static_cast<r32>(dt));
  }

  for (auto& job : m_blendJobs) {
    DoBlendJob(job, static_cast<r32>(dt));
  }

  m_sampleJobs.clear();
}


AnimHandle* Animation::CreateAnimHandle(uuid64 id)
{
  auto it = m_animObjects.find(id);
  if (it != m_animObjects.end()) return nullptr;
  
  AnimHandle* pObj = new AnimHandle(id);
  m_animObjects[id] = pObj;
  return pObj;
}


void Animation::FreeAnimHandle(AnimHandle* pObj)
{
  if (!pObj) return;

  auto it = m_animObjects.find(pObj->_uuid);
  if (it == m_animObjects.end()) return;

  m_animObjects.erase(it);
  delete pObj;
  pObj = nullptr;
}


void Animation::SubmitJob(const AnimJobSubmitInfo& info)
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


Matrix4 Animation::LinearInterpolate(JointPose* currPose, JointPose* nextPose, r32 currTime, r32 nextTime, r32 t)
{
  Matrix4 resultTransform = Matrix4::Identity();
  
  r32 dt = (t - currTime) /  (nextTime - currTime);
  if (isinf(dt)) return resultTransform;

  Vector3 translate = Vector3::Lerp(currPose->_trans, nextPose->_trans, dt);
  Vector3 scale = Vector3::Lerp(currPose->_scale, nextPose->_scale, dt);
  Quaternion rot = Quaternion::Slerp(currPose->_rot, nextPose->_rot, dt);

  Matrix4 mT = Matrix4::Translate(Matrix4(), translate);
  Matrix4 mS = Matrix4::Scale(Matrix4(), scale);
  Matrix4 mR = rot.ToMatrix4();
  resultTransform = mS * mR * mT;

  return resultTransform;
}


void Animation::GetCurrentAndNextPoseIdx(u32* outCurr, u32* outNext, AnimClip* pClip, r32 lt)
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


b32 Animation::EmptyPoseSamples(AnimClip* pClip, i32 currPoseIdx, i32 nextPoseIdx)
{
  if (pClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses.empty()
    || pClip->_aAnimPoseSamples[nextPoseIdx]._aLocalPoses.empty()) {
    return true;
  }
  return false;
}


void Animation::ApplyMorphTargets(AnimHandle* pOutput, AnimClip* pClip, i32 currPoseIdx, i32 nextPoseIdx, r32 lt)
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


void Animation::DoSampleJob(AnimJobSubmitInfo& job, r32 gt)
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
  Skeleton* pSkeleton = Skeleton::GetSkeleton(job._pBaseClip->_skeletonId);

  u32 currPoseIdx = 0;
  u32 nextPoseIdx = 0;

  GetCurrentAndNextPoseIdx(&currPoseIdx, &nextPoseIdx, job._pBaseClip, lt);

  ApplyMorphTargets(job._output, job._pBaseClip, currPoseIdx, nextPoseIdx, lt);

  if (EmptyPoseSamples(job._pBaseClip, currPoseIdx, nextPoseIdx)) { return; }

  AnimPose* currAnimPose = &job._pBaseClip->_aAnimPoseSamples[currPoseIdx];
  AnimPose* nextAnimPose = &job._pBaseClip->_aAnimPoseSamples[nextPoseIdx];

  for (size_t i = 0; i < job._pBaseClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses.size(); ++i) {
    JointPose* currJoint = &currAnimPose->_aLocalPoses[i];
    JointPose* nextJoint = &nextAnimPose->_aLocalPoses[i];
    Matrix4 localTransform = LinearInterpolate(currJoint, nextJoint, currAnimPose->_time, nextAnimPose->_time, lt);
    job._output->_currentPoses[i] = localTransform;
  }

  ApplySkeletonPose(job._output->_finalPalette, job._output->_currentPoses, pSkeleton);
}


void Animation::ApplySkeletonPose(Matrix4* pOutput, Matrix4* pLocalPoses, Skeleton* pSkeleton)
{
  if (!pSkeleton) return;

  for (size_t i = 0; i < pSkeleton->_joints.size(); ++i) {
    Matrix4 parentTransform;
    Matrix4 currentPose;
    u8 parentId = pSkeleton->_joints[i]._iParent;
    if (parentId != Joint::kNoParentId) {
      parentTransform = pLocalPoses[parentId];
    }
    // Now become work space joint matrices
    currentPose = pLocalPoses[i] * parentTransform;
    pLocalPoses[i] = currentPose;
  }

  for (size_t i = 0; i < pSkeleton->_joints.size(); ++i) {
    pOutput[i] = pSkeleton->_joints[i]._InvBindPose * pSkeleton->_joints[i]._invGlobalTransform.Inverse();

  }
}


void Animation::DoBlendJob(AnimJobSubmitInfo& job, r32 gt)
{
  for (auto& layer : job._layers) {
    
  }
}
} // Recluse