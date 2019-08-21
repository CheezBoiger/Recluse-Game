// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Vector3.hpp"
#include "Matrix4.hpp"


namespace Recluse {


// Axis Aligned Bounding Box, based on Erin Catto's Box2D implementation, as well
// as Lester Hedges implementation! Many thanks to them!
struct AABB {
  // Compute the center of the  bounding box.
  void      computeCentroid();
  
  // Compute the surface area of this box. Stored in sA.
  void      computeSurfaceArea();

  B32       overlaps(const AABB& other) const;

  B32       contains(const AABB& other) const;
  
  // Checks if a point is inside this bounding box.
  B32       inside(const Vector3& p) const;

  // Minimum lower bound of the bounding box.
  Vector3     min;

  // Maximum upper bound of the bounding box.
  Vector3     max;

  // Center of the bounding box, otherwise known as a centroid.
  Vector3     centroid;
  
  // Surface area.
  R32         sA;
};
} // Recluse