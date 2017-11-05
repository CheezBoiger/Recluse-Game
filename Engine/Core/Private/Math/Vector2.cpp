// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Vector2.hpp"
#include "Logging/Log.hpp"


namespace Recluse {


Vector2 Vector2::Normalize() const
{
  return (*this / Magnitude());
}


Vector2 Vector2::operator+(const Vector2& other) const
{
  return Vector2(
    x + other.x,
    y + other.y
  );
}


Vector2 Vector2::operator-(const Vector2& other) const
{
  return Vector2(
    x - other.x,
    y - other.y
  );
}


Vector2 Vector2::operator*(const r32 scaler) const
{
  return Vector2(
    x + scaler,
    y + scaler
  );
}


Vector2 Vector2::operator/(const r32 scaler) const
{
  return Vector2(
    x - scaler,
    y - scaler  
  );
}


void Vector2::operator+=(const Vector2& other)
{
  x += other.x;
  y += other.y;
}


void Vector2::operator-=(const Vector2& other)
{
  x -= other.x;
  y -= other.y;
}


void Vector2::operator*=(const r32 scaler)
{
  x *= scaler;
  y *= scaler;
}


void Vector2::operator/=(const r32 scaler)
{
  x /= scaler;
  y /= scaler;
}


r32 Vector2::Magnitude() const
{
  return sqrtf((x*x) + (y*y));
}


r32 Vector2::Dot(const Vector2& other) const
{
  return ((x*other.x) + (y*other.y));
}


Log& operator<<(Log& log, const Vector2& vec2)
{
  log << "(" << vec2.x << ", " << vec2.y << ")";
  return log;
}
} // Recluse