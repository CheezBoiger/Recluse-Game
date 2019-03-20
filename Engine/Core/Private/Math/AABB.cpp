// Copyright (c) 2018 Recluse Project. All rights reserved.
#include "Math/AABB.hpp"
#include "Logging/Log.hpp"

namespace Recluse {


void AABB::computeCentroid()
{
  centroid = Vector3(
    (min.x + max.x) * 0.5f,
    (min.y + max.y) * 0.5f,
    (min.z + max.z) * 0.5f);
}


void AABB::computeSurfaceArea()
{
  r32 dx = max.x - min.x;
  r32 dy = max.y - min.y;
  r32 dz = max.z - min.z;
  sA = 2.0f * (dx * dy + dx * dz + dy * dz);
}


b32 AABB::overlaps(const AABB& other) const
{
  b32 x = (max.x >= other.min.x) && (min.x <= other.max.x);
  b32 y = (max.y >= other.min.y) && (min.y <= other.max.y);
  b32 z = (max.z >= other.min.z) && (min.z <= other.max.z); 
  return (x && y && z);
}


b32 AABB::contains(const AABB& other) const
{
  return false;
}


b32 AABB::inside(const Vector3& p) const
{
  return false;
}
} // Recluse