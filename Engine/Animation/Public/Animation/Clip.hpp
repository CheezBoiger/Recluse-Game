// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Matrix4.hpp"

#include <vector>

#include "Skeleton.hpp"


namespace Recluse {


// The joint pose that represents current position and orientation
// of a joint at a specified key frame. This is in SQT format.
struct JointPose {
  Quaternion  _rot;   // rotation.
  Vector3     _trans; // translation.
  Vector3     _scale; // scale.
  DEBUG_OP(u8          _id);     // node id sample.
};


// A Sample of an animation at some certain key frame. This corresponds to 
// the AnimClip states.
struct AnimPose {
  std::vector<JointPose>  _aLocalPoses;       // local pose matrices at time t.
  std::vector<Matrix4>    _aGlobalPoses;      // global pose matrices at time t.
  std::vector<r32>        _morphs;            // Morph weights at key time t.
  r32                     _time;              // key frame time within the animation.
};


// Single instance of an animation clip. This may represent a "walk," "run,", "shoot," etc...
struct AnimClip {
  AnimClip()
    : _fDuration(0.0f)
    , _fFps(0.0f)
    , _uFrameCount(0)
    , _skeletonId(Skeleton::kNoSkeletonId)
    , _bLooping(false)
    , _name() { }

  // Duration of this animation clip.
  r32                           _fDuration;           // Duration of clip T.
  r32                           _fFps;                // frames per second time.
  u32                           _uFrameCount;         // Number of frames this clip occupies.
  skeleton_uuid_t               _skeletonId;          // id of skeleton that this clip works with.
  std::vector<AnimPose>         _aAnimPoseSamples;    //
  b32                           _bLooping;            //
  std::string                   _name;                // name of this clip.
};


// Animation clip state. Keeps track of the state of an animation's clip.
struct AnimClipState {
  r32                         _fCurrLocalTime;  // Local clock t. Runs until AnimClip._rDuration.
                                                // range : [0, AnimClip._rDuration]
  r32                         _fWeight;
  r32                         _fPlaybackRate;   // rate at which to play back animation.
  b32                         _bEnabled;        // allow enabling this animation clip.
  b32                         _bLooping;        // loop this state.
  i32                         _next;            // Next pose idx within the clip.
  r32                         _tau;             // global start time of this state.
};
} // Recluse