// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Common.hpp"


namespace Recluse {

class Log;

// 2 component vector object.
struct Vector2 {
  struct { r32 x, y; };

  Vector2(r32 x = 0.0f, r32 y = 0.0f) : x(x), y(y) { }
  Vector2(const r32* rawDat)
    : Vector2(rawDat[0], rawDat[1]) { }

  static Vector2 lerp(const Vector2& a, const Vector2& b, r32 t);
  static Vector2 minimum(const Vector2& a, const Vector2& b);
  static Vector2 maximum(const Vector2& a, const Vector2& b);

  Vector2 normalize() const;

  Vector2 operator+(const Vector2& other) const;
  Vector2 operator-(const Vector2& other) const;
  Vector2 operator-() const;

  Vector2 operator*(const Vector2& other) const;
  Vector2 operator*(const r32 scaler) const;
  Vector2 operator/(const r32 scaler) const;

  void    operator+=(const Vector2& other);
  void    operator-=(const Vector2& other);

  void    operator*=(const r32 scaler);
  void    operator/=(const r32 scaler);

  r32     length() const;
  r32     lengthSqr() const;
  r32     dot(const Vector2& other) const;
  r32&    operator [] (const size_t idx) { return (&x)[ idx ]; }

  b8      operator==(const Vector2& other) const; 
  b8      operator!=(const Vector2& other) const;

  friend Vector2 operator/(const r32 scaler, const Vector2& other);
};


Log&      operator<<(Log& log, const Vector2& vec2);
} // Recluse