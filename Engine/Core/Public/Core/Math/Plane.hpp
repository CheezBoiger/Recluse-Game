// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Common.hpp"
#include "Vector4.hpp"

namespace Recluse {


class Log;
// Plane math object, which uses the format equation:
// Ax + By + Cz + D = 0
// Where (A, B, C) define the plane normal vector.
// D defines the distance to the origin.
struct Plane : public Vector4 {
  Plane() { }

  // Use a starting point vec4
  Plane(const Vector4& Start)
    : Vector4(Start) { }

  // Define plane using three points.
  Plane(const Vector3& a, const Vector3& b, const Vector3& c);

  Plane(const Plane& plane)
    : Vector4(plane.x, plane.y, plane.z, plane.w) { }

  Plane(Plane&& plane)
    : Vector4(plane.x, plane.y, plane.z, plane.w) { 
    plane.x = 0.0f; plane.y = 0.0f; plane.z = 0.0f; plane.w = 0.0f;
  }

  Plane& operator=(Plane&& plane) {
    x = plane.x;
    y = plane.y;
    z = plane.z;
    w = plane.w;
    return (*this);
  }

  r32 DistancePoint(const Vector4& Point) const;
  
  Vector3 LineIntersect(const Vector3& p0, const Vector3& p1) const;
  b8      Intersects2Planes(const Plane& other1, const Plane& other2) const;
  b8      IntersectsPlane(const Plane& other) const; 
};


Log& operator<<(Log& log, const Plane& plane);
} // Recluse