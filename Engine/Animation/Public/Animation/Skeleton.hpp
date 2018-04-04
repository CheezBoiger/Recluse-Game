// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Serialize.hpp"

#include <map>

namespace Recluse {



struct Joint 
{
  Matrix4       _mInvBindPose;
  u8            _iParent;
  const tchar*  _name;
};


// Represents the skinned animation of a mesh object in a game.
// The skeleton defines all joints and transformation offsets 
// that make up the animation within the game.
struct Skeleton 
{
  // Full joint transformation that corresponds to a bone.
  Joint*  _aJoints;

  // Number of joins in the skeleton.
  u32     m_uNumOfJoints;
};
} // Recluse