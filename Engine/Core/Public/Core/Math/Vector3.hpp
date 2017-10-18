// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


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

  static Vector3  Lerp(const Vector3& a, const Vector3& b, const r32 t);
  
  Vector3         Normalize() const;
  r32             Dot(const Vector3& other) const;
  Vector3         Cross(const Vector3& other) const;

  Vector3         operator+(const Vector3& other) const;
  Vector3         operator-(const Vector3& other) const;

  Vector3         operator*(const r32 scaler) const;
  Vector3         operator/(const r32 scaler) const;

  void            operator+=(const Vector3& other);
  void            operator-=(const Vector3& other);

  void            operator*=(const r32 scaler);
  void            operator/=(const r32 scaler);

  r32             Magnitude() const;
};
} // Recluse