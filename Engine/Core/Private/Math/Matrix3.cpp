// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Matrix3.hpp"

namespace Recluse {


r32 Matrix3::Determinant() const
{
  return  Data[0][0] * (Data[1][1] * Data[2][2] - Data[1][2] * Data[2][1]) -
          Data[0][1] * (Data[1][0] * Data[2][2] - Data[1][2] * Data[2][0]) +
          Data[0][2] * (Data[1][0] * Data[2][1] - Data[1][1] * Data[2][0]);
}


Matrix3 Matrix3::Transpose() const
{
  return Matrix3(
    Data[0][0], Data[1][0], Data[2][0],
    Data[0][1], Data[1][1], Data[2][1],
    Data[0][2], Data[1][2], Data[2][2]
  );
}


Matrix3 Matrix3::Inverse() const
{
  Matrix3 inverse;
  return inverse;
}


Matrix3 Matrix3::operator*(const Matrix3& other) const
{
  return Matrix3(
    Data[0][0] * other.Data[0][0] + Data[0][1] * other.Data[1][0] + Data[0][2] * other.Data[2][0],
    Data[0][0] * other.Data[0][1] + Data[0][1] * other.Data[1][1] + Data[0][2] * other.Data[2][1],
    Data[0][0] * other.Data[0][2] + Data[0][1] * other.Data[1][2] + Data[0][2] * other.Data[2][2],

    Data[1][0] * other.Data[0][0] + Data[1][1] * other.Data[1][0] + Data[1][2] * other.Data[2][0],
    Data[1][0] * other.Data[0][1] + Data[1][1] * other.Data[1][1] + Data[1][2] * other.Data[2][1],
    Data[1][0] * other.Data[0][2] + Data[1][1] * other.Data[1][2] + Data[1][2] * other.Data[2][2],

    Data[2][0] * other.Data[0][0] + Data[2][1] * other.Data[1][0] + Data[2][2] * other.Data[2][0],
    Data[2][0] * other.Data[0][1] + Data[2][1] * other.Data[1][1] + Data[2][2] * other.Data[2][1],
    Data[2][0] * other.Data[0][2] + Data[2][1] * other.Data[1][2] + Data[2][2] * other.Data[2][2]
  );
}


Matrix3 Matrix3::operator+(const Matrix3& other) const
{
  return Matrix3(
    Data[0][0] + other.Data[0][0], Data[0][1] + other.Data[0][1], Data[0][2] + other.Data[0][2],
    Data[1][0] + other.Data[1][0], Data[1][1] + other.Data[1][1], Data[1][2] + other.Data[1][2],
    Data[2][0] + other.Data[2][0], Data[2][1] + other.Data[2][1], Data[2][2] + other.Data[2][2]
  );
}


Matrix3 Matrix3::operator-(const Matrix3& other) const
{
  return Matrix3(
    Data[0][0] - other.Data[0][0], Data[0][1] - other.Data[0][1], Data[0][2] - other.Data[0][2],
    Data[1][0] - other.Data[1][0], Data[1][1] - other.Data[1][1], Data[1][2] - other.Data[1][2],
    Data[2][0] - other.Data[2][0], Data[2][1] - other.Data[2][1], Data[2][2] - other.Data[2][2]
  );
}


Matrix3 Matrix3::operator*(const r32 scaler) const
{
  return Matrix3(
    Data[0][0] * scaler, Data[0][1] * scaler, Data[0][2] * scaler,
    Data[1][0] * scaler, Data[1][1] * scaler, Data[1][2] * scaler,
    Data[2][0] * scaler, Data[2][1] * scaler, Data[2][2] * scaler
  );
}


r32* Matrix3::operator[](const size_t i)
{
  return Data[i];
}


b8 Matrix3::operator==(const Matrix3& other) const
{
  for (u32 i = 0; i < 3; ++i) {
    for (u32 j = 0; j < 3; ++j) {
      if (Data[i][j] != other.Data[i][j]) {
        return false;
      }
    }
  }
  return true;
}


b8 Matrix3::operator!=(const Matrix3& other) const
{
  return !(*this == other);
}
} // Recluse