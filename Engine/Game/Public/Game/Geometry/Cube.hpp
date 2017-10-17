// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

#include "Renderer/Vertex.hpp"

namespace Recluse {


class Cube {
public:

  static std::vector<SkinnedVertex> MeshInstance();
  static std::vector<u32> IndicesInstance();
};
} // Recluse