// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Math/AABB.hpp"
#include "Logging/Log.hpp"

namespace Recluse {


void AABB::ComputeCentroid()
{
}


b32 AABB::Overlaps(const AABB& other) const
{
  return false;
}


b32 AABB::Contains(const AABB& other) const
{
  return false;
}
} // Recluse