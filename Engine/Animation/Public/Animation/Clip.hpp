// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Matrix4.hpp"

#include <vector>

#include "Skeleton.hpp"

#if defined _DEBUG
#include "Core/Logging/Log.hpp"
#endif


namespace Recluse {


// The joint pose that represents current position and orientation
// of a joint at a specified key frame. This is in SQT format.
struct JointPose {
  Quaternion  _rot;   // rotation.
  Vector3     _trans; // translation.
  Vector3     _scale; // scale.
  U8          _id;    // node id sample.

  JointPose() 
    : _scale(Vector3(1.0f, 1.0f, 1.0f))
    , _rot(Quaternion())
    , _trans(Vector3()) { }

#if defined _DEBUG
  friend Log& operator<<(Log& l, JointPose& p) {
    l << "Node ID: " << p._id; 
    return l;
  }
#endif
};


// A Sample of an animation at some certain key frame. This corresponds to 
// the AnimClip states.
struct AnimPose {
  std::vector<JointPose>  _aLocalPoses;       // local pose matrices at time t.
  std::vector<Matrix4>    _aGlobalPoses;      // global pose matrices at time t.
  std::vector<R32>        _morphs;            // Morph weights at key time t.
  R32                     _time;              // key frame time within the animation.
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
  R32                           _fDuration;           // Duration of clip T.
  R32                           _fFps;                // frames per second time.
  U32                           _uFrameCount;         // Number of frames this clip occupies.
  skeleton_uuid_t               _skeletonId;          // id of skeleton that this clip works with.
  std::vector<AnimPose>         _aAnimPoseSamples;    //
  B32                           _bLooping;            //
  std::string                   _name;                // name of this clip.
};


// Animation clip state. Keeps track of the state of an animation's clip.
struct AnimClipState {
  R32                         _fCurrLocalTime;  // Local clock t. Runs until AnimClip._rDuration.
                                                // range : [0, AnimClip._rDuration]
  R32                         _fWeight;
  R32                         _fPlaybackRate;   // rate at which to play back animation.
  B32                         _bEnabled;        // allow enabling this animation clip.
  B32                         _bLooping;        // loop this state.
  I32                         _next;            // Next pose idx within the clip.
  R32                         _tau;             // global start time of this state.
};
} // Recluse