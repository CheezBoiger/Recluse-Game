// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Common.hpp"


namespace Recluse {

class Log;

// 2 component vector object.
struct Vector2 {
  struct { R32 x, y; };

  Vector2(R32 x = 0.0f, R32 y = 0.0f) : x(x), y(y) { }
  Vector2(const R32* rawDat)
    : Vector2(rawDat[0], rawDat[1]) { }

  static Vector2 lerp(const Vector2& a, const Vector2& b, R32 t);
  static Vector2 minimum(const Vector2& a, const Vector2& b);
  static Vector2 maximum(const Vector2& a, const Vector2& b);

  Vector2 normalize() const;

  Vector2 operator+(const Vector2& other) const;
  Vector2 operator-(const Vector2& other) const;
  Vector2 operator-() const;

  Vector2 operator*(const Vector2& other) const;
  Vector2 operator*(const R32 scaler) const;
  Vector2 operator/(const R32 scaler) const;

  void    operator+=(const Vector2& other);
  void    operator-=(const Vector2& other);

  void    operator*=(const R32 scaler);
  void    operator/=(const R32 scaler);

  R32     length() const;
  R32     lengthSqr() const;
  R32     dot(const Vector2& other) const;
  R32&    operator [] (const size_t idx) { return (&x)[ idx ]; }

  B8      operator==(const Vector2& other) const; 
  B8      operator!=(const Vector2& other) const;

  friend Vector2 operator/(const R32 scaler, const Vector2& other);
};


Log&      operator<<(Log& log, const Vector2& vec2);
} // Recluse