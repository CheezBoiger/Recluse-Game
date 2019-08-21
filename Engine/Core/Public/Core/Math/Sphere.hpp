// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Common.hpp"
#include "Core/Math/Vector3.hpp"

namespace Recluse {


struct Sphere {
  Vector3     center;
  R32         radius;

  Sphere(Vector3 center = Vector3(), R32 radius = 1.0f)
    : center(center), radius(radius) { }

  B8  intersectsLine(const Vector3& p0, const Vector3& p1) const;
  B8  intersectsPoint(const Vector3& p) const;
};
} // Recluse