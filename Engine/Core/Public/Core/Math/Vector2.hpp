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

  Vector2 Normalize() const;

  Vector2 operator+(const Vector2& other) const;
  Vector2 operator-(const Vector2& other) const;
  Vector2 operator-() const;

  Vector2 operator*(const r32 scaler) const;
  Vector2 operator/(const r32 scaler) const;

  void    operator+=(const Vector2& other);
  void    operator-=(const Vector2& other);

  void    operator*=(const r32 scaler);
  void    operator/=(const r32 scaler);

  r32     Magnitude() const;
  r32     Dot(const Vector2& other) const;
  r32&    operator [] (const size_t idx) { return (&x)[ idx ]; }

  b8      operator==(const Vector2& other) const; 
  b8      operator!=(const Vector2& other) const;
};


Log&      operator<<(Log& log, const Vector2& vec2);
} // Recluse