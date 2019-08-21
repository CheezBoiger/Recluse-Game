// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Vector4.hpp"
#include "Math/Matrix4.hpp"
#include "Logging/Log.hpp"
#include <math.h>

#if defined _M_X64 && __USE_INTEL_INTRINSICS__
#define FAST_INTRINSICS 1
#include <xmmintrin.h>
#endif

namespace Recluse {


Vector4 Vector4::lerp(const Vector4& A, const Vector4& B, const R32 T)
{
  return A * (1.0f - T) + B * T;
}


Vector4 Vector4::minimum(const Vector4& a, const Vector4& b)
{
  return Vector4(
    b.x < a.x ? b.x : a.y,
    b.y < a.y ? b.y : a.y,
    b.z < a.z ? b.z : a.z,
    b.w < a.w ? b.w : a.w
  );
}


Vector4 Vector4::maximum(const Vector4& a, const Vector4& b)
{
  return Vector4(
    a.x < b.x ? b.x : a.x,
    a.y < b.y ? b.y : a.y,
    a.z < b.z ? b.z : a.z,
    a.w < b.w ? b.w : a.w
  );
}


Vector4 Vector4::operator+(const Vector4& other) const
{
#if FAST_INTRINSICS
  __m128 a0 = _mm_load_ps(&x);
  __m128 b0 = _mm_load_ps(&other.x);
  __m128 ans = _mm_add_ps(a0, b0);
  Vector4 v1; _mm_store_ps(&v1.x, ans);
  return v1;
#else
  return Vector4(
    x + other.x, y + other.y, z + other.z, w + other.z
  );
#endif
}


Vector4 Vector4::operator-(const Vector4& other) const
{
#if FAST_INTRINSICS
  __m128 a0 = _mm_load_ps(&x);
  __m128 b0 = _mm_load_ps(&other.x);
  __m128 ans = _mm_sub_ps(a0, b0);
  Vector4 v1; _mm_store_ps(&v1.x, ans);
  return v1;
#else
  return Vector4(
    x - other.x, y - other.y, z - other.z, w - other.w
  );
#endif
}


Vector4 Vector4::operator-() const 
{
  return Vector4(
    -x, -y, -z, -w
  );
}


Vector4 Vector4::operator*(const R32 scaler) const
{
  return Vector4(
    x * scaler, y * scaler, z * scaler, w * scaler
  );
}


Vector4 Vector4::operator/(const R32 scaler) const
{
  R32 scale = 1.0f / scaler;
  return Vector4(
    x * scale, y * scale, z * scale, w * scale
  );
}


void Vector4::operator+=(const Vector4& other)
{
#if FAST_INTRINSICS
  __m128 v0 = _mm_load_ps(&x);
  __m128 v1 = _mm_load_ps(&other.x);
  __m128 ans = _mm_add_ps(v0, v1);
  _mm_store_ps(&x, ans);
#else
  x += other.x;
  y += other.y;
  z += other.z;
  w += other.w;
#endif
}


void Vector4::operator-=(const Vector4& other)
{
#if FAST_INTRINSICS
  __m128 v0 = _mm_load_ps(&x);
  __m128 v1 = _mm_load_ps(&other.x);
  __m128 ans = _mm_sub_ps(v0, v1);
  _mm_store_ps(&x, ans);
#else
  x -= other.x;
  y -= other.y;
  z -= other.z;
  w -= other.w;
#endif
}


void Vector4::operator*=(const R32 scaler)
{
  x *= scaler;
  y *= scaler;
  z *= scaler;
  w *= scaler;
}


void Vector4::operator/=(const R32 scaler)
{
  R32 scale = 1.0f / scaler;
  x *= scale;
  y *= scale;
  z *= scale;
  w *= scale;
}


R32 Vector4::length() const
{
  return sqrtf( (x * x) + (y * y) + (z * z) + (w * w) );
}


R32 Vector4::lengthSqr() const
{
  return (x * x) + (y * y) + (z * z) + (w * w);
}


R32 Vector4::dot(const Vector4& other) const
{
  return (x * other.x + y * other.y + z * other.z + w * other.w);
}


Vector4 Vector4::normalize() const
{
  R32 magnitude = length();
  return (*this) / magnitude;
}


B8 Vector4::operator==(const Vector4& other) const
{
  if (x == other.x && 
      y == other.y && 
      z == other.z && 
      w == other.w
     ) 
  {
    return true;
  }

  return false;
}


B8 Vector4::operator!=(const Vector4& other) const
{
  return !(*this == other);
}


Vector4 Vector4::operator*(const Matrix4& other) const
{
  return Vector4(
    other.Data[0][0] * x + other.Data[1][0] * y + other.Data[2][0] * z + other.Data[3][0] * w,
    other.Data[0][1] * x + other.Data[1][1] * y + other.Data[2][1] * z + other.Data[3][1] * w,
    other.Data[0][2] * x + other.Data[1][2] * y + other.Data[2][2] * z + other.Data[3][2] * w,
    other.Data[0][3] * x + other.Data[1][3] * y + other.Data[2][3] * z + other.Data[3][3] * w
  );
}


Log& operator<<(Log& log, const Vector4& vec4)
{
  log << "(" << vec4.x << ", " << vec4.y << ", " << vec4.z << ", " << vec4.w << ")";
  return log;
}
} // Recluse