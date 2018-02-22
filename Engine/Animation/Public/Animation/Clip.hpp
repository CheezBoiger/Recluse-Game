// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Matrix3.hpp"
#include "Core/Math/Matrix4.hpp"

#include <vector>


namespace Recluse {


struct TranslationKey {
  r32           _rTimeStamp;
  u32           _uJointTrack;
  Vector3       _vTranslation;
};


struct RotationKey {
  r32           _rTimeStamp;
  u32           _uJointTrack;
  Quaternion    _qRotation;
};


struct ScaleKey {
  r32           _rTimeStamp;
  u32           _uJointTrack;
  Vector3       _vScale;
};


// Single instance of an animation clip. This may represent a "walk," "run,", "shoot," etc...
struct AnimationClip {
  // Duration of this animation clip.
  r32                           _rDuration;

  r32                           _rNumJointTracks;

  // Transformation keys, held in a data oriented manner. These
  // containers represent key frames in an animation clip.
  std::vector<TranslationKey>   _Tranlations;
  std::vector<RotationKey>      _Rotations;
  std::vector<ScaleKey>         _Scales;
};
} // Recluse