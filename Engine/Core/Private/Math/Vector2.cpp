// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Vector2.hpp"
#include "Logging/Log.hpp"


namespace Recluse {


Vector2 Vector2::Normalize() const
{
  return (*this / Length());
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


Vector2 Vector2::operator-() const
{
  return Vector2(
    -x, -y
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
  r32 scale = 1.0f / scaler;
  return Vector2(
    x * scale,
    y * scale  
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
  r32 scale = 1.0f / scaler;
  x *= scale;
  y *= scale;
}


r32 Vector2::Length() const
{
  return sqrtf((x*x) + (y*y));
}


r32 Vector2::LengthSqr() const
{
  return (x * x) + (y * y);
}


r32 Vector2::Dot(const Vector2& other) const
{
  return ((x*other.x) + (y*other.y));
}


b8 Vector2::operator==(const Vector2& other) const
{
  if (x == other.x &&
      y == other.y 
     )
  {
    return true;
  }
  
  return false;
}


b8 Vector2::operator!=(const Vector2& other) const
{
  return !(*this == other);
}


Log& operator<<(Log& log, const Vector2& vec2)
{
  log << "(" << vec2.x << ", " << vec2.y << ")";
  return log;
}
} // Recluse