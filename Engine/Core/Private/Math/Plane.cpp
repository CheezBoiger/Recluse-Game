// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Plane.hpp"
#include "Logging/Log.hpp"

namespace Recluse {


Plane::Plane(const Vector3& a, const Vector3& b, const Vector3& c)
{
  Vector3 aux1, aux2;
  aux1 = a - b;
  aux2 = c - b;

  Vector3 Normal = aux2 ^ aux1;
  Normal = Normal.Normalize();

  x = Normal.x;
  y = Normal.y;
  z = Normal.z;
  
  Vector3 Point = b;
  w = -(Normal.Dot(Point));
}


Log& operator<<(Log& log, const Plane& plane)
{
  log << "Normal: (" << plane.x << ", " << plane.y << ", " << plane.z << ") D: " << plane.w;
  return log;
}
} // Recluse