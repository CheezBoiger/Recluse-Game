// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Vector3.hpp"
#include "Math/Matrix3.hpp"
#include "Math/Ray.hpp"
#include "Logging/Log.hpp"
#include <cmath>

namespace Recluse {


Vector3 Vector3::UP = Vector3(0.0f, 1.0f, 0.0f);
Vector3 Vector3::DOWN = Vector3(0.0f, -1.0f, 0.0f);
Vector3 Vector3::LEFT = Vector3(-1.0f, 0.0f, 0.0f);
Vector3 Vector3::RIGHT = Vector3(1.0f, 0.0f, 0.0f);
Vector3 Vector3::FRONT = Vector3(0.0f, 0.0f, 1.0f);
Vector3 Vector3::BACK = Vector3(0.0f, 0.0f, -1.0f);
Vector3 Vector3::ZERO = Vector3(0.0f, 0.0f, 0.0f);


Vector3 Vector3::Lerp(const Vector3& a, const Vector3& b, const r32 t)
{
  return a * (1.0f - t) + b * t;
}


Vector3 Vector3::Min(const Vector3& a, const Vector3& b)
{
  return Vector3( b.x < a.x ? b.x : a.y,
                  b.y < a.y ? b.y : a.y,
                  b.z < a.z ? b.z : a.z); 
}


Vector3 Vector3::Max(const Vector3& a, const Vector3& b)
{
  return Vector3( a.x < b.x ? b.x : a.x,
                  a.y < b.y ? b.y : a.y,
                  a.z < b.z ? b.z : a.z);
}


r32 Vector3::Dot(const Vector3& other) const
{
  return (x * other.x + y * other.y + z * other.z);
}


//   i j k
//c  x y z
//o  x y z
Vector3 Vector3::Cross(const Vector3& other) const
{
  return Vector3(
    y * other.z - z * other.y,
    z * other.x - x * other.z,
    x * other.y - y * other.x
  );
}


Vector3 Vector3::Normalize() const
{
  r32 magnitude = Length();
  return (*this) / magnitude;
}


r32 Vector3::Length() const
{
  return sqrtf(x * x + y * y + z * z);
}


r32 Vector3::LengthSqr() const
{
  return (x * x) + (y * y) + (z * z);
}


Vector3 Vector3::operator+(const Vector3& other) const
{
  return Vector3(
    x + other.x,
    y + other.y,
    z + other.z
  );
}


Vector3 Vector3::operator-(const Vector3& other) const
{
  return Vector3(
    x - other.x,
    y - other.y,
    z - other.z
  );
}


Vector3 Vector3::operator-() const
{
  return Vector3(
    -x, -y, -z
  );
}


Vector3 Vector3::operator*(const r32 scaler) const
{
  return Vector3(
    x * scaler,
    y * scaler,
    z * scaler
  );
}


Vector3 Vector3::operator/(const r32 scaler) const
{
  r32 scale = 1.0f / scaler;
  return Vector3(
    x * scale,
    y * scale,
    z * scale
  );
}


void Vector3::operator+=(const Vector3& other)
{
  x += other.x;
  y += other.y;
  z += other.z;
}


void Vector3::operator-=(const Vector3& other)
{
  x -= other.x;
  y -= other.y;
  z -= other.z;
}


void Vector3::operator*=(const r32 scaler)
{
  x *= scaler;
  y *= scaler;
  z *= scaler;
}


void Vector3::operator/=(const r32 scaler)
{
  r32 scale = 1.0f / scaler;
  x *= scale;
  y *= scale;
  z *= scale;
}


b8 Vector3::operator==(const Vector3& other) const
{
  if (x == other.x &&
      y == other.y &&
      z == other.z
     )
  {
    return true;
  }

  return false;
}


b8 Vector3::operator!=(const Vector3& other) const
{
  return !(*this == other);
}


Vector3 Vector3::operator*(const Matrix3& other) const 
{
  return Vector3(
    other.Data[0][0] * x + other.Data[1][0] * y + other.Data[2][0] * z,
    other.Data[0][1] * x + other.Data[1][1] * y + other.Data[2][1] * z,
    other.Data[0][2] * x + other.Data[1][2] * y + other.Data[2][2] * z
  );
}


Vector3 Vector3::operator*(const Vector3& scaler) const 
{
  return Vector3(x * scaler.x, y * scaler.y, z * scaler.z);
}


void Vector3::operator*=(const Vector3& scaler)
{
  x *= scaler.x;
  y *= scaler.y;
  z *= scaler.z;
}


Log& operator<<(Log& log, const Vector3& vec3)
{
  log << "(" << vec3.x << ", " << vec3.y << ", " << vec3.z << ")";
  return log;
}


Log& operator<<(Log& log, const Ray& ray)
{
  log << "Origin:    " << ray.Origin << " "
      << "Direction: " << ray.Direction; 
  return log;
}
} // Recluse