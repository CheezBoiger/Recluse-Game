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
struct JointPose 
{
  Quaternion  _rot;
  Vector3     _trans;
  Vector3     _scale;
};


// A Sample of an animation at some certain key frame.
struct AnimSample 
{
  JointPose*  _aLocalPoses;
  Matrix4*    _aGlobalPoses;
};


// Single instance of an animation clip. This may represent a "walk," "run,", "shoot," etc...
struct AnimationClip 
{
  // Duration of this animation clip.
  r32                           _rDuration;
  r32                           _fps;
  u32                           _frameCount;
  u32                           _skeletonId;
  AnimSample*                   _aAnimSamples;
  u32                           _bLooping;
};
} // Recluse