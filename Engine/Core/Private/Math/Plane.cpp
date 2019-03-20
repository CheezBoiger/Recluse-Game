// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Plane.hpp"
#include "Logging/Log.hpp"

namespace Recluse {


Plane::Plane(const Vector3& a, const Vector3& b, const Vector3& c)
{
  Vector3 aux1, aux2;
  aux1 = a - b;
  aux2 = c - b;

  Vector3 getNormal = aux2 ^ aux1;
  getNormal = getNormal.normalize();

  x = getNormal.x;
  y = getNormal.y;
  z = getNormal.z;
  
  Vector3 Point = b;
  w = -(getNormal.dot(Point));
}


Log& operator<<(Log& log, const Plane& plane)
{
  log << "Normal: (" << plane.x << ", " << plane.y << ", " << plane.z << ") D: " << plane.w;
  return log;
}
} // Recluse