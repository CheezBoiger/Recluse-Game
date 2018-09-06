// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector2.hpp"

namespace Recluse {

class Log;
struct Matrix3;

// Math vector object of 3 components <x, y, z>.
struct Vector3 {
  union { struct { r32 x, y, z; };
          struct { r32 r, g, b; };
          struct { r32 s, t, p; }; };

  static Vector3  UP;
  static Vector3  DOWN;
  static Vector3  LEFT;
  static Vector3  RIGHT;
  static Vector3  FRONT;
  static Vector3  BACK;
  static Vector3  ZERO;

  Vector3(r32 x = 0.0f, r32 y = 0.0f, r32 z = 0.0f)
    : x(x), y(y), z(z) { }

  Vector3(const Vector2& other, r32 z = 0.0f)
    : x(other.x), y(other.y), z(z) { }

  Vector3(const r32* rawDat)
    : Vector3(rawDat[0], rawDat[1], rawDat[2]) 
  { }

  static Vector3  Lerp(const Vector3& a, const Vector3& b, const r32 t);
  static Vector3  Min(const Vector3& a, const Vector3& b);
  static Vector3  Max(const Vector3& a, const Vector3& b);
  
  Vector3         Normalize() const;
  r32             Dot(const Vector3& other) const;
  Vector3         Cross(const Vector3& other) const;

  Vector3         operator+(const Vector3& other) const;
  Vector3         operator-(const Vector3& other) const;
  Vector3         operator-() const;

  Vector3         operator*(const r32 scaler) const;
  Vector3         operator*(const Vector3& scale) const; // Component-wise scaling.
  Vector3         operator/(const r32 scaler) const;

  // Vector3 to matrix3 multiplication in row major order.
  Vector3         operator*(const Matrix3& other) const;

  void            operator+=(const Vector3& other);
  void            operator-=(const Vector3& other);

  void            operator*=(const r32 scaler);
  void            operator*=(const Vector3& scaler);  // Component-wise scaling.
  void            operator/=(const r32 scaler);

  // Cross product expressed as the exterior product of this vector and other.
  // This can also be denoted as a wedge product in exterior algebra.
  Vector3         operator^(const Vector3& other) const { return Cross(other); }

  // Dot producted denoted as bitwise OR.
  r32             operator|(const Vector3& other) const { return Dot(other); }

  r32             Length() const;
  r32             LengthSqr() const;
  r32&            operator [] (const size_t idx) { return (&x)[ idx ]; }

  b8              operator==(const Vector3& other) const;
  b8              operator!=(const Vector3& other) const;
};

// Overload to let Log take in vector3 values.
Log&              operator<<(Log& log, const Vector3& vec3);
} // Recluse