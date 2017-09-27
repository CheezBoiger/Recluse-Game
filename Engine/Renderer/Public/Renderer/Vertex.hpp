// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


struct Vertex {
  r32 position    [4];
  r32 normal      [4];
  r32 texcoord    [2];
  r32 padding     [2];
  r32 color       [4];
};


struct QuadVertex {
  r32 position [2];
  r32 padding  [2];
  r32 texcoord [4];
};


struct SkinnedVertex {
  r32 position      [4];
  r32 normal        [4];
  r32 texcoord0     [2];
  r32 texcoord1     [2];
  r32 color         [4];
  r32 boneWeights   [4];
  i32 boneIds       [4];
};
} // Recluse