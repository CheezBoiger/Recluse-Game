// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Core/Math/Vector4.hpp"
#include "Core/Types.hpp"

namespace Recluse {


struct Color4 {
  struct { u8 r, g, b, a; };

  
  Color4(const Vector4& other);

  Color4(u8 r = 0, u8 g = 0, u8 b = 0, u8 a = 255)
    : r(r), g(g), b(b), a(a) { }

  Vector4 ToVector4() const;

  operator Vector4 () {
    return Vector4();
  }
};
} // Recluse