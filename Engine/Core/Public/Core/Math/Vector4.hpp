// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"
#include "Vector3.hpp"

namespace Recluse {


struct Matrix4;
class Log;

// Math object of 4 components. formatted as (x,y,z,w)
struct Vector4 {
  struct { R32 x, y, z, w; };

  Vector4(R32 x = 0.0f, R32 y = 0.0f, R32 z = 0.0f, R32 w = 0.0f)
    : x(x), y(y), z(z), w(w) { }
  Vector4(const Vector3& other, R32 w = 0.0f)
    : Vector4(other.x, other.y, other.z, w)  { }

  Vector4(const Vector2& other, R32 z = 0.0f, R32 w = 0.0f)
    : Vector4(other.x, other.y, z, w) { }

  Vector4(const R32* rawDat)
    : Vector4(rawDat[0], rawDat[1], rawDat[2], rawDat[3]) 
  { }

  static Vector4  lerp(const Vector4& p0, const Vector4& p1, const R32 t);
  static Vector4 minimum(const Vector4& a, const Vector4& b);
  static Vector4 maximum(const Vector4& a, const Vector4& b);

  Vector4         normalize() const;

  Vector4         operator+(const Vector4& other) const;
  Vector4         operator-(const Vector4& other) const;
  Vector4         operator-() const;

  Vector4         operator*(const R32 scaler) const;
  Vector4         operator/(const R32 scaler) const;
  
  void            operator+=(const Vector4& other);
  void            operator-=(const Vector4& other);

  void            operator*=(const R32 scaler);
  void            operator/=(const R32 scaler);

  R32&            operator[](size_t idx) { return (&x)[ idx ]; }

  // Vector4 to Matrix4 multiplication in row major order.
  Vector4         operator*(const Matrix4& other) const;

  R32             length() const;
  R32             lengthSqr() const;
  R32             dot(const Vector4& other) const;

  B8              operator==(const Vector4& other) const;
  B8              operator!=(const Vector4& other) const;
};

// Overload to let Log take in vector3 values.
Log&              operator<<(Log& log, const Vector4& vec3);
} // Recluse