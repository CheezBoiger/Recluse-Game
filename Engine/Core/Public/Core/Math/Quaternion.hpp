// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Vector3.hpp"
#include "Matrix4.hpp"

namespace Recluse {


struct Quaternion {
  struct { r32 x, y, z, w; };


  Quaternion(r32 x = 0.0f, r32 y = 0.0f, r32 z = 0.0f, r32 w = 1.0f)
    : x(x), y(y), z(z), w(w) { }

  static Quaternion AngleAxis(const r32 radians, const Vector3& axis);
  static Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, const r32 t);
  static Quaternion EulerAnglesToQuaternion(const Vector3& euler);
  static Quaternion Matrix4ToQuaternion(const Matrix4& rot);  
  
  Quaternion        operator+(const Quaternion& other) const;
  Quaternion        operator-(const Quaternion& other) const;
  Quaternion        operator*(const Quaternion& other) const;

  Quaternion        operator*(const r32 scaler) const;
  Quaternion        operator/(const r32 scaler) const;
  Quaternion        operator-() const;

  void              operator*=(const Quaternion& other);
  void              operator+=(const Quaternion& other);
  void              operator-=(const Quaternion& other);

  void              operator*=(const r32 scaler);
  void              operator/=(const r32 scaler);

  Quaternion        Conjugate() const;
  Quaternion        Inverse() const;
  Quaternion        Normalize() const;

  Vector3           ToEulerAngles() const;
  Matrix4           ToMatrix4() const;

  r32               Norm() const;
};
} // Recluse