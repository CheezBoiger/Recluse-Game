// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Serialize.hpp"
#include "Core/Math/Matrix4.hpp"
#include <map>

namespace Recluse {

typedef u32      anim_uuid_t;


struct Joint {
  Matrix4       _InvBindPose;
  const tchar*  _name;
  u8            _iParent;
};


// Represents the skinned animation of a mesh object in a game.
// The skeleton defines all joints and transformation offsets 
// that make up the animation within the game.
struct Skeleton {
  // Full joint transformation that corresponds to a bone.
  Joint*          _aJoints;

  // Number of joins in the skeleton.
  u32             _uNumOfJoints;
  
  // This skeleton's unique id.
  anim_uuid_t     _uuid;
};
} // Recluse