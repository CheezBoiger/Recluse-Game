// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

#include "Core/Math/Vector3.hpp"
#include "Renderer/Vertex.hpp"

namespace Recluse {


class Cube {
public:

  static std::vector<StaticVertex> meshInstance(r32 scale = 1.0f);
  static std::vector<u32> indicesInstance();
  static const Vector3 minimum;
  static const Vector3 maximum;
};
} // Recluse