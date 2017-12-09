// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

#include "Renderer/Vertex.hpp"

namespace Recluse {


class Cube {
public:

  static std::vector<StaticVertex> MeshInstance(r32 scale = 1.0f);
  static std::vector<u32> IndicesInstance();
};
} // Recluse