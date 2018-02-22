// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Clip.hpp"
#include "Core/Serialize.hpp"

#include <map>

namespace Recluse {


// Represents the skinned animation of a mesh object in a game.
// The skeleton defines all joints and transformation offsets 
// that make up the animation within the game.
class Skeleton {
public:

private:
  // The bones ids used to represent each bone influencing a certain
  // vertex in a mesh.
  std::vector<u32>                      m_BoneIdx;

  // Full joint transformation that corresponds to a bone.
  std::vector<Matrix4>                  m_JointTransforms;

  // Animation clips used by this skeleton.
  std::map<std::string, AnimationClip>  m_AnimationClips;
};
} // Recluse