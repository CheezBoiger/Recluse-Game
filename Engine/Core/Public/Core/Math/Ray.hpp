// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Vector3.hpp"


namespace Recluse {

class Log;

struct Ray {
  Vector3     Origin;
  Vector3     Direction;


  Ray(const Vector3& origin = Vector3(), const Vector3& direction = Vector3())
    : Origin(origin), Direction(direction) { }


  // Obtain a vector going along this ray direction.
  inline Vector3 point(r32 t) {
    return Origin + Direction * t;
  }
};


Log& operator<<(Log& log, const Ray& ray);
} // Recluse 