// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Clip.hpp"
#include "Core/Types.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"
#include "Core/Math/Common.hpp"

#include "UI/UI.hpp"

#include <assert.h>
namespace Recluse {


sampler_id_t AnimSampler::kSamplerCount = 0;


AnimSampler::AnimSampler()
  : _pClip(nullptr)
  , _output(64)
  , _currPoseIdx(0)
  , _nextPoseIdx(1)
  , _samplerId(kSamplerCount++)
  , _tauS(0.0f)
{
  _state._bEnabled          = true;
  _state._bLooping          = true;
  _state._fCurrLocalTime    = 0.0f;
  _state._fPlaybackRate     = 1.0f;
  _state._fWeight           = 1.0f;
}


void AnimSampler::Step(r32 gt)
{
  if (!_state._bEnabled) return;
  R_ASSERT(_tauS >= 0.0f, "tau is negative.");
  R_ASSERT(_pClip, "No clip for this sampler.");
  r32 R = _state._fPlaybackRate;
  r32 t = (gt - _tauS) * R;

  if (t > _pClip->_fDuration) {
    if (_state._bLooping) {
      // Reset _tauS.
      Play(gt);
    }
    t = t - _pClip->_fDuration;
    _currPoseIdx = 0;
    _nextPoseIdx = (_pClip->_aAnimPoseSamples.size() > 1 ? 1 : 0);
  }

  _state._fCurrLocalTime = t;
  // TODO(): Now find the local poses of t.
  Skeleton* skeleton = Skeleton::GetSkeleton(_pClip->_skeletonId);
  b32 rootInJoints = skeleton->_rootInJoints;

  // We don't have to clean out all C matrices, just the root. All others will be 
  // flushed out from here.
  {
    Matrix4 localTransform = LinearInterpolate(t, 0);
    if (rootInJoints) {
      _output[0] = localTransform;
    }
    _globalTransform = localTransform;
  }

  for (size_t i = 1; i < _pClip->_aAnimPoseSamples[_currPoseIdx]._aLocalPoses.size(); ++i) {
    size_t idx = (rootInJoints ? i : i - 1);
    Matrix4 localTransform = LinearInterpolate(t, i);
    Matrix4 parentTransform;
    u8 parentId = skeleton->_joints[idx]._iParent;
    if (parentId == 0xff) {
      parentTransform = _globalTransform;
    } else {
      parentTransform = _output[skeleton->_joints[idx]._iParent];
    }
    // Combine result and parent matrices to produce the current C pose. This is in world joint space.
    _output[idx] = localTransform * parentTransform;
  }

  ApplyCurrentPose(skeleton);
}


u32 AnimSampler::GetPaletteSz()
{
  R_ASSERT(_pClip, "Clip is null for this sampler.");
  return (u32 )_pClip->_aAnimPoseSamples[_currPoseIdx]._aLocalPoses.size();
}


void AnimSampler::ApplyCurrentPose(Skeleton* skeleton)
{
  // Multiplay matrices by their inverse bind to transform our vertices to local joint space.
  for (size_t i = 0; i < skeleton->_joints.size(); ++i) {
    _output[i] = skeleton->_joints[i]._InvBindPose * _output[i] * skeleton->_rootInvTransform;
  }
}


Matrix4 AnimSampler::LinearInterpolate(r32 t, size_t i)
{
  AnimPose* currPose = &_pClip->_aAnimPoseSamples[_currPoseIdx];
  AnimPose* nextPose = &_pClip->_aAnimPoseSamples[_nextPoseIdx];
  JointPose& currJointPose = currPose->_aLocalPoses[i];
  JointPose& nextJointPose = nextPose->_aLocalPoses[i];

  r32 dt = (t - currPose->_time) / ((nextPose->_time - currPose->_time));

  // Clip current pose is moving to next key pose.
  if (dt > 1.0f) {
    _currPoseIdx = _nextPoseIdx;
    _nextPoseIdx += 1;
    if (_nextPoseIdx >= _pClip->_aAnimPoseSamples.size()) _nextPoseIdx = 0;
  }

  Matrix4 resultTransform = Matrix4::Identity();
  if (!isinf(dt)) {
    Vector3 trans = Vector3::Lerp(currJointPose._trans, nextJointPose._trans, dt);
    Vector3 scale = Vector3::Lerp(currJointPose._scale, nextJointPose._scale, dt);
    Quaternion rot = Quaternion::Slerp(currJointPose._rot, nextJointPose._rot, dt);
    Matrix4 _T = Matrix4::Translate(Matrix4(), trans);
    Matrix4 _S = Matrix4::Scale(Matrix4(), scale);
    Matrix4 _R = rot.ToMatrix4();
    resultTransform = _S * _R * _T;
  }
  return resultTransform;
}


void AnimSampler::ReadClip()
{
  _state._bLooping = _pClip->_bLooping;
}
} // Recluse