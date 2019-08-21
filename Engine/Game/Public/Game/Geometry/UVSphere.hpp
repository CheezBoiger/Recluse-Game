// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Utility/Vector.hpp"

#include "Renderer/Vertex.hpp"

namespace Recluse {


class UVSphere {
public:
  static std::vector<StaticVertex> meshInstance(R32 radius, U32 sliceCnt, U32 stckCount);
  static std::vector<U32> indicesInstance(U32 verticesCnt, U32 sliceCnd, U32 stckCnt);
};
} // Recluse