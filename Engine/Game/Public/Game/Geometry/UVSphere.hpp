// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

#include "Renderer/Vertex.hpp"

namespace Recluse {


class UVSphere {
public:
  static std::vector<StaticVertex> meshInstance(r32 radius, u32 sliceCnt, u32 stckCount);
  static std::vector<u32> indicesInstance(u32 verticesCnt, u32 sliceCnd, u32 stckCnt);
};
} // Recluse