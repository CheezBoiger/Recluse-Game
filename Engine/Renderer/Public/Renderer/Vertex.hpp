// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Vector3.hpp"
#include "Core/Math/Vector2.hpp"

namespace Recluse {


struct Vertex {
  Vector4 position;
  Vector4 normal;
  Vector2 texcoord0;
  Vector2 texcoord1;
};


struct QuadVertex {
  Vector2 position;  
  Vector2 texcoord;
};


struct SkinnedVertex {
  Vector4 position;
  Vector4 normal;
  Vector2 texcoord0;
  Vector2 texcoord1;
  Vector4 boneWeights;
  i32     boneIds       [4];
};
} // Recluse