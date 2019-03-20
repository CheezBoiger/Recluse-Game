// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Matrix3.hpp"
#include "Math/Matrix4.hpp"
#include "Logging/Log.hpp"
#include "Exception.hpp"

#include <iomanip>

namespace Recluse {


r32 Matrix3::determinant() const
{
  return  Data[0][0] * (Data[1][1] * Data[2][2] - Data[1][2] * Data[2][1]) -
          Data[0][1] * (Data[1][0] * Data[2][2] - Data[1][2] * Data[2][0]) +
          Data[0][2] * (Data[1][0] * Data[2][1] - Data[1][1] * Data[2][0]);
}


Matrix3 Matrix3::transpose() const
{
  return Matrix3(
    Data[0][0], Data[1][0], Data[2][0],
    Data[0][1], Data[1][1], Data[2][1],
    Data[0][2], Data[1][2], Data[2][2]
  );
}


Matrix3 Matrix3::inverse() const
{
  // Calculate similar to 4x4 matrix.
  // Get 1 / determinant
  // Calculate cofactor matrix -> adjugate
  //  Multiply Adjugate * 1/determinant
  Matrix3 inverse;
  R_ASSERT(false, "Not implemented.");
  return inverse;
}


Matrix3 Matrix3::adjugate() const
{
  Matrix3 cofactor;
  R_ASSERT(false, "Not implemented.");
  return  cofactor.transpose();
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


Log& operator<<(Log& log, const Matrix3& mat3)
{
  for (size_t row = 0; row < 3; ++row) {
    for (size_t col = 0; col < 3; ++col) {
      log << std::setw(7) << mat3(row, col);
    }
    log << "\n";
  }
  return log;
}


Matrix3::Matrix3(const Matrix4& other)
{
  Data[0][0] = other.Data[0][0]; Data[0][1] = other.Data[0][1]; Data[0][2] = other.Data[0][2];
  Data[1][0] = other.Data[1][0]; Data[1][1] = other.Data[1][1]; Data[1][2] = other.Data[1][2];
  Data[2][0] = other.Data[2][0]; Data[2][1] = other.Data[2][1]; Data[2][2] = other.Data[2][2];
}


Matrix3 Matrix3::ToMatrix3(const Matrix4& other)
{
  return Matrix3(other);
}
} // Recluse