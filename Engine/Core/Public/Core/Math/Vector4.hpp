// Copyright (c) 2017 Recluse Project.
#pragma once


#include "Core/Types.hpp"

namespace Recluse {


struct Matrix4;

struct Vector4 {
  struct { r32 x, y, z, w; };

  Vector4(r32 x = 0.0f, r32 y = 0.0f, r32 z = 0.0f, r32 w = 1.0f)
    : x(x), y(y), z(z), w(w) { }

};
} // Recluse