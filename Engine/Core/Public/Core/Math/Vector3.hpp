// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector2.hpp"

namespace Recluse {

class Log;

struct Vector3 {
  struct { r32 x, y, z; };

  static Vector3  UP;
  static Vector3  DOWN;
  static Vector3  LEFT;
  static Vector3  RIGHT;
  static Vector3  FRONT;
  static Vector3  BACK;

  Vector3(r32 x = 0.0f, r32 y = 0.0f, r32 z = 0.0f)
    : x(x), y(y), z(z) { }

  Vector3(const Vector2& other, r32 z = 0.0f)
    : x(other.x), y(other.y), z(z) { }

  static Vector3  Lerp(const Vector3& a, const Vector3& b, const r32 t);
  
  Vector3         Normalize() const;
  r32             Dot(const Vector3& other) const;
  Vector3         Cross(const Vector3& other) const;

  Vector3         operator+(const Vector3& other) const;
  Vector3         operator-(const Vector3& other) const;
  Vector3         operator-() const;

  Vector3         operator*(const r32 scaler) const;
  Vector3         operator/(const r32 scaler) const;

  void            operator+=(const Vector3& other);
  void            operator-=(const Vector3& other);

  void            operator*=(const r32 scaler);
  void            operator/=(const r32 scaler);

  // Cross product expressed as the exterior product of this vector and other.
  // This can also be denoted as a wedge product in exterior algebra.
  Vector3         operator^(const Vector3& other) const { return Cross(other); }

  r32             operator|(const Vector3& other) const { return Dot(other); }
  r32             Magnitude() const;
  r32&            operator [] (const size_t idx) { return (&x)[ idx ]; }

  b8              operator==(const Vector3& other) const;
  b8              operator!=(const Vector3& other) const;
};

// Overload to let Log take in vector3 values.
Log&              operator<<(Log& log, const Vector3& vec3);
} // Recluse