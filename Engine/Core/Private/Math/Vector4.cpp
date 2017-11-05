// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Vector4.hpp"
#include "Logging/Log.hpp"
#include <math.h>


namespace Recluse {


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


Vector4 Vector4::operator*(const r32 scaler) const
{
  return Vector4(
    x * scaler, y * scaler, z * scaler, w * scaler
  );
}


Vector4 Vector4::operator/(const r32 scaler) const
{
  return Vector4(
    x / scaler, y / scaler, z / scaler, w / scaler
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
  x /= scaler;
  y /= scaler;
  z /= scaler;
  w /= scaler;
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


Log& operator<<(Log& log, const Vector4& vec4)
{
  log << "(" << vec4.x << ", " << vec4.y << ", " << vec4.z << ", " << vec4.w << ")";
  return log;
}
} // Recluse