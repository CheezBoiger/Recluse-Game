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
// of a joint at a specified key frame.
struct JointPose {
  Quaternion  _rot;
  Vector4     _trans;
  r32         _scale; // uniform scale.
};


// A Sample of an animation at some certain key frame.
struct AnimSample {
  JointPose*  _aLocalPoses;
  Matrix4*    _aGlobalPoses;
};


// Single instance of an animation clip. This may represent a "walk," "run,", "shoot," etc...
struct AnimClip {
  // Duration of this animation clip.
  r32                           _rDuration;
  r32                           _fps;
  u32                           _frameCount;
  Skeleton*                     _pSkeleton;
  AnimSample*                   _aAnimSamples;
  b32                           _bLooping;
};


// Animation clip state. Keeps track of the state of an animation's clip.
struct AnimClipState {
  std::string                 _animClipName;
  r32                         _rTimePos;
  r32                         _rWeight;
  r32                         _rPlaybackRate;
  b32                         _bEnabled;
  b32                         _bLooping;
};
} // Recluse