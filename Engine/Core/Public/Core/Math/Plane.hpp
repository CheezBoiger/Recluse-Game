// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Common.hpp"
#include "Vector4.hpp"

namespace Recluse {


// Plane math object, which uses the format equation:
// Ax + By + Cz + D = 0
struct Plane : public Vector4 {
  Plane(const Vector4& Start);
  Plane(const Plane&);
  Plane(Plane&&);

  r32 DistancePoint(const Vector4& Point) const;
  
  Vector3 LineIntersect(const Vector3& p0, const Vector3& p1) const;
  b8      Intersects2Planes(const Plane& other1, const Plane& other2) const;
  b8      IntersectsPlane(const Plane& other) const; 
};
} // Recluse