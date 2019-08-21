// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Math/Vector2.hpp"

namespace Recluse {

class Log;
struct Matrix3;
struct Quaternion;

// Math vector object of 3 components <x, y, z>.
struct Vector3 {
  union { struct { R32 x, y, z; };
          struct { R32 r, g, b; };
          struct { R32 s, t, p; }; };

  static Vector3  UP;
  static Vector3  DOWN;
  static Vector3  LEFT;
  static Vector3  RIGHT;
  static Vector3  FRONT;
  static Vector3  BACK;
  static Vector3  ZERO;
  static Vector3  ONE;

  Vector3(R32 x = 0.0f, R32 y = 0.0f, R32 z = 0.0f)
    : x(x), y(y), z(z) { }

  Vector3(const Vector2& other, R32 z = 0.0f)
    : x(other.x), y(other.y), z(z) { }

  Vector3(const R32* rawDat)
    : Vector3(rawDat[0], rawDat[1], rawDat[2]) 
  { }

  static Vector3  lerp(const Vector3& a, const Vector3& b, const R32 t);
  static Vector3  minimum(const Vector3& a, const Vector3& b);
  static Vector3  maximum(const Vector3& a, const Vector3& b);
  
  Vector3         normalize() const;
  R32             dot(const Vector3& other) const;
  Vector3         cross(const Vector3& other) const;

  Vector3         operator+(const Vector3& other) const;
  Vector3         operator-(const Vector3& other) const;
  Vector3         operator-() const;

  Vector3         operator*(const R32 scaler) const;
  Vector3         operator*(const Vector3& scale) const; // Component-wise scaling.
  Vector3         operator/(const R32 scaler) const;

  // Vector3 to matrix3 multiplication in row major order.
  Vector3         operator*(const Matrix3& other) const;
  Vector3         operator*(const Quaternion& other) const;

  void            operator+=(const Vector3& other);
  void            operator-=(const Vector3& other);

  void            operator*=(const R32 scaler);
  void            operator*=(const Vector3& scaler);  // Component-wise scaling.
  void            operator/=(const R32 scaler);

  // Cross product expressed as the exterior product of this vector and other.
  // This can also be denoted as a wedge product in exterior algebra.
  Vector3         operator^(const Vector3& other) const { return cross(other); }

  // Dot producted denoted as bitwise OR.
  R32             operator|(const Vector3& other) const { return dot(other); }

  R32             length() const;
  R32             lengthSqr() const;
  R32&            operator [] (const size_t idx) { return (&x)[ idx ]; }

  B8              operator==(const Vector3& other) const;
  B8              operator!=(const Vector3& other) const;
};

// Overload to let Log take in vector3 values.
Log&              operator<<(Log& log, const Vector3& vec3);
} // Recluse