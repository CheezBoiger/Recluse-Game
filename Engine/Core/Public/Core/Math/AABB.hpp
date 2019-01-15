// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Vector3.hpp"
#include "Matrix4.hpp"


namespace Recluse {


// Axis Aligned Bounding Box, based on Erin Catto's Box2D implementation, as well
// as Lester Hedges implementation! Many thanks to them!
struct AABB {
  // Compute the center of the  bounding box.
  void      ComputeCentroid();
  
  // Compute the surface area of this box. Stored in sA.
  void      ComputeSurfaceArea();

  b32       Overlaps(const AABB& other) const;

  b32       Contains(const AABB& other) const;
  
  // Checks if a point is inside this bounding box.
  b32       Inside(const Vector3& p) const;

  // Minimum lower bound of the bounding box.
  Vector3     min;

  // Maximum upper bound of the bounding box.
  Vector3     max;

  // Center of the bounding box, otherwise known as a centroid.
  Vector3     centroid;
  
  // Surface area.
  r32         sA;
};
} // Recluse