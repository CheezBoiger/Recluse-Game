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
      AnimSampleJob job = { };
      job._pClip = info._pBaseClip;
      job._clipState = { };
      job._clipState._bLooping = info._pBaseClip->_bLooping;
      job._clipState._fCurrLocalTime = 0.0f;
      job._clipState._bEnabled = true;
      job._clipState._fCurrLocalTime = info._timeRatio * info._pBaseClip->_fDuration;
      job._clipState._fPlaybackRate = info._playbackRate;
      job._sz = 64;
      m_sampleJobs.push_back(job);
      m_sampleJobMaps[info._uuid] = &m_sampleJobs.back();
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


void Animation::DoSampleJob(AnimSampleJob& job, r32 gt)
{
  if (!job._clipState._bEnabled) { return; }
  r32 tau = job._clipState._tau;
  r32 rate = job._clipState._fPlaybackRate;
  r32 lt = job._clipState._fCurrLocalTime + gt * rate;
  if (lt > job._pClip->_fDuration) {
    lt -= job._pClip->_fDuration;
    job._clipState._tau = gt;
  }
  if (lt < 0.0f) {
    lt = job._pClip->_fDuration + lt;
    if (lt < 0.0f) {
      lt += job._pClip->_fDuration;
      job._clipState._tau = gt;
    }
  }
  job._clipState._fCurrLocalTime = lt;
  Skeleton * pSkeleton = Skeleton::GetSkeleton(job._pClip->_skeletonId);
  Matrix4* palette = job._output;
  u32 paletteSz = job._sz;

  u32 currPoseIdx = 0;
  u32 nextPoseIdx = 0;

  for (size_t i = 0; i < job._pClip->_aAnimPoseSamples.size(); ++i) {
    AnimPose& pose = job._pClip->_aAnimPoseSamples[i];
    if (pose._time > lt) {
      currPoseIdx = static_cast<u32>(i) - 1u;
      nextPoseIdx = static_cast<u32>(i);
      break;
    }
  }

  if (pSkeleton) {
    Matrix4 globalTransform;
    b32 rootInJoints = pSkeleton->_rootInJoints;
    {
      Matrix4 localTransform = LinearInterpolate(
        &job._pClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses[0],
        &job._pClip->_aAnimPoseSamples[nextPoseIdx]._aLocalPoses[0],
        job._pClip->_aAnimPoseSamples[currPoseIdx]._time,
        job._pClip->_aAnimPoseSamples[nextPoseIdx]._time,
        lt);
      if (rootInJoints) {
        job._output[0] = localTransform;
      }
      globalTransform = localTransform;
    }

    AnimPose* currAnimPose = &job._pClip->_aAnimPoseSamples[currPoseIdx];
    AnimPose* nextAnimPose = &job._pClip->_aAnimPoseSamples[nextPoseIdx];

    for (size_t i = 1; i < job._pClip->_aAnimPoseSamples[currPoseIdx]._aLocalPoses.size(); ++i) {
      size_t idx = (rootInJoints ? i : i - 1);
      JointPose* currJoint = &currAnimPose->_aLocalPoses[i];
      JointPose* nextJoint = &nextAnimPose->_aLocalPoses[i];
      Matrix4 localTransform = LinearInterpolate(currJoint, nextJoint, currAnimPose->_time, nextAnimPose->_time, lt);
      Matrix4 parentTransform;
      u8 parentId = pSkeleton->_joints[idx]._iParent;
      if (parentId == Joint::kNoParentId) {
        parentTransform = globalTransform;
      }
      else {
        parentTransform = job._output[pSkeleton->_joints[idx]._iParent];
      }
      job._output[idx] = localTransform * parentTransform;
    }

    ApplySkeletonPose(job._output, pSkeleton);
  }
  else {
    // No skeleton found for this clip, assume heirachy none. Renderer should take care of parent child heirarchy.
  }
}


void Animation::ApplySkeletonPose(Matrix4* pOutput, Skeleton* pSkeleton)
{
  if (!pSkeleton) return;

  for (size_t i = 0; i < pSkeleton->_joints.size(); ++i) {
    pOutput[i] = pSkeleton->_joints[i]._InvBindPose * pOutput[i] * pSkeleton->_rootInvTransform;
  }
}


void Animation::DoBlendJob(AnimBlendJob& job, r32 gt)
{
  for (auto& layer : job._layers) {
    
  }
}


AnimSampleJob* Animation::GetCurrentSampleJob(uuid64 uuid)
{
  AnimSampleJob* sample = nullptr;
  auto it = m_sampleJobMaps.find(uuid);
  if (it != m_sampleJobMaps.end()) sample = it->second;
  return sample;
}
} // Recluse