// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Clip.hpp"
#include "Core/Types.hpp"
#include "Core/Logging/Log.hpp"
#include "Core/Exception.hpp"


namespace Recluse {


AnimSampler::AnimSampler()
  : _pClip(nullptr)
  , m_output(128)
  , _prevPoseIdx(0)
  , _nextPoseIdx(0)
{
  _state._bEnabled          = true;
  _state._bLooping          = true;
  _state._fCurrLocalTime    = 0.0f;
  _state._fPlaybackRate     = 0.1f;
  _state._fWeight           = 1.0f;
}


void AnimSampler::Step(r32 gt)
{
  if (!_state._bEnabled) return;

  R_ASSERT(_pClip, "No clip for this sampler.");
  r32 R = _state._fPlaybackRate;
  r32 t = (gt - _tauS) * R;
  if (_state._bLooping && t > _pClip->_fDuration) {
    Play(gt);
    t = t - _pClip->_fDuration;
  }
  _state._fCurrLocalTime = t;

  // TODO(): Now find the local poses of t.
  Skeleton& skeleton = Skeleton::GetSkeleton(_pClip->_skeletonId);
  AnimPose* prevPose = &_pClip->_aAnimPoseSamples[_prevPoseIdx];
  AnimPose* nextPose = &_pClip->_aAnimPoseSamples[_nextPoseIdx];
  if (nextPose->_time < t) {
    _prevPoseIdx = _nextPoseIdx;
    _nextPoseIdx += 1;
    if (_nextPoseIdx >= _pClip->_aAnimPoseSamples.size()) {
      _nextPoseIdx = 0;
    }
    prevPose = &_pClip->_aAnimPoseSamples[_prevPoseIdx];
    nextPose = &_pClip->_aAnimPoseSamples[_nextPoseIdx];
  }
  
  for (size_t i = 0; i < _pClip->_aAnimPoseSamples[_prevPoseIdx]._aLocalPoses.size(); ++i) {
    JointPose& prevJointPose = prevPose->_aLocalPoses[i];
    JointPose& nextJointPose = nextPose->_aLocalPoses[i];
    Matrix4 prevGlobalTransform = _pClip->_aAnimPoseSamples[_prevPoseIdx]._aGlobalPoses[i];
    Matrix4 nextGlobalTransform = _pClip->_aAnimPoseSamples[_nextPoseIdx]._aGlobalPoses[i];

    r32 dt = (t - prevPose->_time) / ((nextPose->_time - prevPose->_time));
    R_ASSERT(!std::isinf(dt), "infinite");
    Vector3 trans = Vector3::Lerp(prevJointPose._trans, nextJointPose._trans, dt);
    Vector3 scale = Vector3::Lerp(prevJointPose._scale, nextJointPose._scale, dt);
    Quaternion rot = Quaternion::Slerp(prevJointPose._rot, nextJointPose._rot, dt);

    Matrix4 _T = Matrix4::Translate(Matrix4(), trans);
    Matrix4 _S = Matrix4::Scale(Matrix4(), scale);
    Matrix4 _R = rot.ToMatrix4();

    // TODO(): Need to fix this as transform works, but adding the inv bind pose causes issues.
    // Need to multiplay the parent matrix as well!
    Matrix4 resultTransform = _R * _T;

    m_output[i] = resultTransform * skeleton._joints[i]._InvBindPose;
  }
}
} // Recluse