// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Common.hpp"

#include "Logging/Log.hpp"
#include "Exception.hpp"
#include <cmath>

namespace Recluse {


r32 Quaternion::Norm() const
{
  return sqrtf(x*x + y*y + z*z + w*w);
}


Quaternion Quaternion::Conjugate() const
{
  return Quaternion(-x, -y, -z, w);
}


Quaternion Quaternion::Inverse() const
{
  Quaternion conjugate = Conjugate();
  r32 norm2 = (x*x + y*y + z*z + w*w);
  conjugate /= norm2;
  return conjugate;
}


Quaternion Quaternion::operator-() const
{
  return Quaternion(-x, -y, -z, -w);
}


Quaternion Quaternion::operator*(const Quaternion& other) const
{
  return Quaternion(
    (w * other.x) + (x * other.w) + (y * other.z) - (z * other.y),
    (w * other.y) - (x * other.z) + (y * other.w) + (z * other.x),
    (w * other.z) + (x * other.y) - (y * other.x) + (z * other.w),
    (w * other.w) - (x * other.x) - (y * other.y) - (z * other.z)
  );
}


void Quaternion::operator*=(const Quaternion& other)
{
  Quaternion ori = *this;
  x = (ori.w * other.x) + (ori.x * other.w) + (ori.y * other.z) - (ori.z * other.y);
  y = (ori.w * other.y) - (ori.x * other.z) + (ori.y * other.w) + (ori.z * other.x);
  z = (ori.w * other.z) + (ori.x * other.y) - (ori.y * other.x) + (ori.z * other.w);
  w = (ori.w * other.w) - (ori.x * other.x) - (ori.y * other.y) - (ori.z * other.z);
}


Quaternion Quaternion::operator*(const r32 scaler) const
{
  return Quaternion(x * scaler, y * scaler, z * scaler, w * scaler);
}


Quaternion Quaternion::operator/(const r32 scaler) const
{
  return Quaternion(x / scaler, y / scaler, z / scaler, w / scaler);
}


Quaternion Quaternion::operator+(const Quaternion& other) const 
{
  return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w);
}


Quaternion Quaternion::operator-(const Quaternion& other) const
{
  return Quaternion(x - other.x, y - other.y, z - other.z, w - other.w);
}


void Quaternion::operator*=(const r32 scaler) 
{
  x *= scaler;
  y *= scaler;
  z *= scaler;
  w *= scaler;
}


void Quaternion::operator/=(const r32 scaler)
{
  x /= scaler;
  y /= scaler;
  z /= scaler;
  w /= scaler;
}


void Quaternion::operator+=(const Quaternion& other)
{
  x += other.x;
  y += other.y;
  z += other.z;
  w += other.w;
}


void Quaternion::operator-=(const Quaternion& other)
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  w -= other.w;
}


Quaternion Quaternion::Normalize() const
{
  r32 norm = Norm();
  return (*this / norm);
}


Quaternion Quaternion::AngleAxis(const r32 radians, const Vector3& axis)
{
  r32 radHalf = radians * 0.5f;
  r32 sineHalf = sinf(radHalf);
  return Quaternion(
    axis.x * sineHalf,
    axis.y * sineHalf,
    axis.z * sineHalf,
    cosf(radHalf)
  );
}


Quaternion Quaternion::EulerAnglesToQuaternion(const Vector3& euler)
{
  Quaternion q;
  r32 t0 = cosf(euler.x * 0.5f);
  r32 t1 = sinf(euler.x * 0.5f);
  r32 t2 = cosf(euler.y * 0.5f);
  r32 t3 = sinf(euler.y * 0.5f);
  r32 t4 = cosf(euler.z * 0.5f);
  r32 t5 = sinf(euler.z * 0.5f);

  q.w = t0*t2*t4 + t1*t3*t5;
  q.x = t0*t3*t4 - t1*t2*t5;
  q.y = t0*t2*t5 + t1*t3*t4;
  q.z = t1*t2*t4 - t0*t3*t5;

  return q;
}


Vector3 Quaternion::ToEulerAngles() const
{
  Vector3 eulerAngles;
  r32 ysqrt = y * y;

  r32 t0 = 2.0f * (w * x + y * z);
  r32 t1 = 1.0f - 2.0f * (x * x + ysqrt);
  eulerAngles.x = atan2f(t0, t1);

  r32 t2 = 2.0f * (w * y - z * x);
  t2 = t2 > 1.0f ? 1.0f : t2;
  t2 = t2 < -1.0f ? -1.0f : t2;
  eulerAngles.y = asinf(t2);

  r32 t3 = 2.0f * (w * z + x * y);
  r32 t4 = 1.0f - 2.0f * (ysqrt + z * z);
  eulerAngles.z = atan2f(t3, t4);

  return eulerAngles;
}


Quaternion Quaternion::Matrix4ToQuaternion(const Matrix4& rot)
{
  R_DEBUG(rError, __FUNCTION__ "Not implemented.\n");
  return Quaternion();
}


Matrix4 Quaternion::ToMatrix4() const
{
  return Matrix4(
    1.0f - 2.0f*(y*y + z*z),  2.0f*(x*y + w*z),         2.0f*(x*z - w*y),         0.0f,
    2.0f*(x*y - w*z),         1.0f - 2.0f*(x*x + z*z),  2.0f*(y*z + w*x),         0.0f,
    2.0f*(x*z + w*y),         2.0f*(y*z - w*x),         1.0f - 2.0f*(x*x + y*y),  0.0f,
    0.0f,                     0.0f,                     0.0f,                     1.0f
  );
}


Quaternion Quaternion::Slerp(const Quaternion& q0, const Quaternion& q1, const r32 t)
{
  r32 dot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;

  const r32 kThreshold = 0.9995f;
  if (fabs(dot) > kThreshold) {
    Quaternion result = q0 + (q1 - q0) * t;
    result = result.Normalize();
    return result;
  }
  
  Quaternion v1 = q1;
  if (dot < 0.0f) {
    v1 = -q1;
    dot = -dot;
  }

  Clamp(dot, -1, 1);
  r32 theta0 = acosf(dot);
  r32 theta = theta0 * t;
  Quaternion v2 = v1 - q0*dot;
  v2 = v2.Normalize();

  return q0*cosf(theta) + v2*sinf(theta);
}


b8 Quaternion::operator==(const Quaternion& other) const
{
  if (x == other.x && y == other.y && z == other.z && w == other.w)
    return true;
  return false;
}


b8 Quaternion::operator!=(const Quaternion& other) const
{
  return !( *this == other );
}


Log& operator<<(Log& log, const Quaternion& quat)
{
  log << "(" << quat.w << ", " << quat.x << ", " << quat.y << ", " << quat.z << ")";
  return log;
}
} // Recluse