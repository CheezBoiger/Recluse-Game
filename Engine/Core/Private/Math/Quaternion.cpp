// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Core/Math/Quaternion.hpp"
#include "Core/Math/Common.hpp"

#include "Core/Math/Vector3.hpp"
#include "Core/Math/Matrix4.hpp"

#include "Logging/Log.hpp"
#include "Exception.hpp"
#include <cmath>

namespace Recluse {


Quaternion Quaternion::lookRotation(const Vector3& dir, const Vector3& upwards)
{
  // Implementation from 
  // https://gist.github.com/aeroson/043001ca12fe29ee911e
  // modified for left hand coordinate systems that is Recluse.
  Vector3 forward = dir.normalize();
  Vector3 right = upwards.Cross(forward).normalize();
  Vector3 up = forward.Cross(right);

  r32 m00 = right.x;
  r32 m01 = right.y;
  r32 m02 = right.z;
  r32 m10 = up.x;
  r32 m11 = up.y;
  r32 m12 = up.z;
  r32 m20 = forward.x;
  r32 m21 = forward.y;
  r32 m22 = forward.z;

  r32 num8 = (m00 + m11) + m22;
  Quaternion ret;

  if (num8 > 0.0f) {
    r32 num = sqrtf(num8 + 1.0f);
    ret.w = num * 0.5f;
    num = 0.5f / num;
    ret.x = (m12 - m21) * num;
    ret.y = (m20 - m02) * num;
    ret.z = (m01 - m10) * num;
    return ret;
  }

  if ((m00 >= m11) && (m00 >= m22)) {
    r32 num7 = sqrtf(((1.0f + m00) - m11) - m22);
    r32 num4 = 0.5f * num7;
    ret.x = 0.5f * num7;
    ret.y = (m01 + m10) * num4;
    ret.z = (m02 + m20) * num4;
    ret.w = (m12 - m21) * num4;
    return ret;
  }

  if (m11 > m22) {
    r32 num6 = sqrtf(((1.0f + m11) - m00) - m22);
    r32 num3 = 0.5f / num6;
    ret.x = (m10 + m01) * num3;
    ret.y = 0.5f * num6;
    ret.z = (m21 + m12) * num3;
    ret.w = (m20 - m02) * num3;
    return ret;
  }

  r32 num5 = sqrtf(((1.0f + m22) - m00) - m11);
  r32 num2 = 0.5f / num5;
  ret.x = (m20 + m02) * num2;
  ret.y = (m21 + m12) * num2;
  ret.z = 0.5f * num5;
  ret.w = (m01 - m10) * num2;
  return ret;
}


r32 Quaternion::norm() const
{
  return sqrtf(x*x + y*y + z*z + w*w);
}


Quaternion Quaternion::Conjugate() const
{
  return Quaternion(-x, -y, -z, w);
}


Quaternion Quaternion::inverse() const
{
  Quaternion conjugate = Conjugate();
  r32 norm2 = (x*x + y*y + z*z + w*w);
  // TODO(): Possible divide by zero: may want to check for issue here.
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


Quaternion Quaternion::normalize() const
{
  r32 n = norm();
  return (*this / n);
}


Quaternion Quaternion::angleAxis(const r32 radians, const Vector3& axis)
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


Quaternion Quaternion::eulerAnglesToQuaternion(const Vector3& euler)
{
  Quaternion q;
  r32 t0 = cosf(euler.z * 0.5f);
  r32 t1 = sinf(euler.z * 0.5f);
  r32 t2 = cosf(euler.x * 0.5f);
  r32 t3 = sinf(euler.x * 0.5f);
  r32 t4 = cosf(euler.y * 0.5f);
  r32 t5 = sinf(euler.y * 0.5f);

  q.w = t0*t2*t4 + t1*t3*t5;
  q.x = t0*t3*t4 - t1*t2*t5;
  q.y = t0*t2*t5 + t1*t3*t4;
  q.z = t1*t2*t4 - t0*t3*t5;

  return q;
}


Vector3 Quaternion::toEulerAngles() const
{
  Vector3 eulerAngles;
  r32 ysqrt = y * y;

  // roll
  r32 t0 = 2.0f * (w * x +  y * z);
  r32 t1 = 1.0f - 2.0f * (x * x + ysqrt);
  eulerAngles.x = atan2f(t0, t1);

  // pitch
  r32 t2 = 2.0f * (w * y - x * z);
  t2 = t2 > 1.0f ? 1.0f : t2;
  t2 = t2 < -1.0f ? -1.0f : t2;
  eulerAngles.y = asinf(t2);

  // yaw
  r32 t3 = 2.0f * (w * z + x * y);
  r32 t4 = 1.0f - 2.0f * (ysqrt + z * z);
  eulerAngles.z = atan2f(t3, t4);

  return eulerAngles;
}


Quaternion Quaternion::matrix4ToQuaternion(const Matrix4& rot)
{
  R_DEBUG(rError, __FUNCTION__ "Not implemented.\n");
  return Quaternion();
}


Matrix4 Quaternion::toMatrix4() const
{
  return Matrix4(
    1.0f - 2.0f*(y*y + z*z),  2.0f*(x*y + w*z),         2.0f*(x*z - w*y),         0.0f,
    2.0f*(x*y - w*z),         1.0f - 2.0f*(x*x + z*z),  2.0f*(y*z + w*x),         0.0f,
    2.0f*(x*z + w*y),         2.0f*(y*z - w*x),         1.0f - 2.0f*(x*x + y*y),  0.0f,
    0.0f,                     0.0f,                     0.0f,                     1.0f
  );
}


Quaternion Quaternion::slerp(const Quaternion& q0, const Quaternion& q1, const r32 t)
{
  r32 dot = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;

  const r32 kThreshold = 0.9995f;
  if (fabs(dot) > kThreshold) {
    Quaternion result = q0 + (q1 - q0) * t;
    result = result.normalize();
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
  v2 = v2.normalize();

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


Vector3 Quaternion::operator*(const Vector3& other) const
{
  Vector3 u(x, y, z);
  r32 s = w;
  
  return u * u.dot(other) * 2.0f + other * (s * s - u.dot(u)) + u.Cross(other) * 2.0f * s;
}


Log& operator<<(Log& log, const Quaternion& quat)
{
  log << "(" << quat.w << ", " << quat.x << ", " << quat.y << ", " << quat.z << ")";
  return log;
}
} // Recluse