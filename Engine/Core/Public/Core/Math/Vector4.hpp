// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"

namespace Recluse {


struct Matrix4;

struct Vector4 {
  struct { r32 x, y, z, w; };

  Vector4(r32 x = 0.0f, r32 y = 0.0f, r32 z = 0.0f, r32 w = 0.0f)
    : x(x), y(y), z(z), w(w) { }

  Vector4 Normalize() const;

  Vector4 operator+(const Vector4& other) const;
  Vector4 operator-(const Vector4& other) const;

  Vector4 operator*(const r32 scaler) const;
  Vector4 operator/(const r32 scaler) const;
  
  void    operator+=(const Vector4& other);
  void    operator-=(const Vector4& other);

  void    operator*=(const r32 scaler);
  void    operator/=(const r32 scaler);
  
  r32     Magnitude() const;
  r32     Dot(const Vector4& other) const;
};
} // Recluse