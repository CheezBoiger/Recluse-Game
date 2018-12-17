// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

namespace Recluse {


class Log;
struct Matrix4;
struct Vector3;

// Quaternion mathematical object for spatial rotations.
// Quaternions of this library are formatted as XYZW form.
struct Quaternion {
  struct { r32 x, y, z, w; };

  Quaternion(r32 x = 0.0f, r32 y = 0.0f, r32 z = 0.0f, r32 w = 1.0f)
    : x(x), y(y), z(z), w(w) { }

  Quaternion(const r32* raw)
    : x(raw[0]), y(raw[1]), z(raw[2]), w(raw[3]) { }

  static Quaternion AngleAxis(const r32 radians, const Vector3& axis);
  static Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, const r32 t);
  static Quaternion EulerAnglesToQuaternion(const Vector3& euler);
  static Quaternion Matrix4ToQuaternion(const Matrix4& rot);  
  static Quaternion LookRotation(const Vector3&dir, const Vector3& up);
  static Quaternion Identity() { return Quaternion(); }
  
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

  b8                operator==(const Quaternion& other) const;
  b8                operator!=(const Quaternion& other) const;

  Quaternion        Conjugate() const;
  Quaternion        Inverse() const;
  Quaternion        Normalize() const;

  // Converts quaternion components to euler angles with components:
  // x -> pitch
  // y -> yaw
  // z -> roll
  Vector3           ToEulerAngles() const;

  // Rotates a point (other) about this quaternion.
  Vector3           operator*(const Vector3& other) const;

  Matrix4           ToMatrix4() const;

  r32               Norm() const;
};


// Quaternions are read as (w, x, y, z).
Log&                operator<<(Log& log, const Quaternion& quat);
} // Recluse