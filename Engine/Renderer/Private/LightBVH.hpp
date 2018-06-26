// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Math/AABB.hpp"
#include "Core/Math/Ray.hpp"
#include "Core/Types.hpp"

#include <vector>


namespace Recluse {


class LightBVH {
public:
  struct Node {
    AABB  _aabb;
    u32   _parent;
    u32   _left;
    u32   _right;
  };

private:
  // World bounding box.
  AABB      m_worldAABB;
  
  // Index of the root node, should always be 0.
  u32       m_rootIdx;

  std::vector<LightBVH::Node> m_nodes;
};
} // Recluse