// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Vector4.hpp"
#include "Math/Matrix4.hpp"
#include "Logging/Log.hpp"
#include <math.h>


namespace Recluse {


Vector4 Vector4::Lerp(const Vector4& A, const Vector4& B, const r32 T)
{
  return A * (1.0f - T) + B * T;
}


Vector4 Vector4::operator+(const Vector4& other) const
{
  return Vector4(
    x + other.x, y + other.y, z + other.z, w + other.z
  );
}


Vector4 Vector4::operator-(const Vector4& other) const
{
  return Vector4(
    x - other.x, y - other.y, z - other.z, w - other.w
  );
}


Vector4 Vector4::operator-() const 
{
  return Vector4(
    -x, -y, -z, -w
  );
}


Vector4 Vector4::operator*(const r32 scaler) const
{
  return Vector4(
    x * scaler, y * scaler, z * scaler, w * scaler
  );
}


Vector4 Vector4::operator/(const r32 scaler) const
{
  r32 scale = 1.0f / scaler;
  return Vector4(
    x * scale, y * scale, z * scale, w * scale
  );
}


void Vector4::operator+=(const Vector4& other)
{
  x += other.x;
  y += other.y;
  z += other.z;
  w += other.w;
}


void Vector4::operator-=(const Vector4& other)
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
  w -= other.w;
}


void Vector4::operator*=(const r32 scaler)
{
  x *= scaler;
  y *= scaler;
  z *= scaler;
  w *= scaler;
}


void Vector4::operator/=(const r32 scaler)
{
  r32 scale = 1.0f / scaler;
  x *= scale;
  y *= scale;
  z *= scale;
  w *= scale;
}


r32 Vector4::Magnitude() const
{
  return sqrtf( (x * x) + (y * y) + (z * z) + (w * w) );
}


r32 Vector4::Dot(const Vector4& other) const
{
  return (x * other.x + y * other.y + z * other.z + w * other.w);
}


Vector4 Vector4::Normalize() const
{
  r32 magnitude = Magnitude();
  return (*this) / magnitude;
}


b8 Vector4::operator==(const Vector4& other) const
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


b8 Vector4::operator!=(const Vector4& other) const
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