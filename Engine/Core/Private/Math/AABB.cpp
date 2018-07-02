// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Math/AABB.hpp"
#include "Logging/Log.hpp"

namespace Recluse {


void AABB::ComputeCentroid()
{
  centroid = Vector3(
    (min.x + max.x) * 0.5f,
    (min.y + max.y) * 0.5f,
    (min.z + max.z) * 0.5f);
}


void AABB::ComputeSurfaceArea()
{
  r32 dx = max.x - min.x;
  r32 dy = max.y - min.y;
  r32 dz = max.z - min.z;
  sA = 2.0f * (dx * dy + dx * dz + dy * dz);
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