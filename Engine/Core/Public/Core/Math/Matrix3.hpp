// Copyright (c) Recluse Project. All rights reserved.
#pragma once


#include "Core/Types.hpp"


namespace Recluse {

class Log;
struct Matrix4;
struct Vector3;


struct Matrix3 {
  R32         Data[3][3];

  static Matrix3 identity() {
    return Matrix3();
  }

  Matrix3(R32 a00 = 1.0f, R32 a01 = 0.0f, R32 a02 = 0.0f,
          R32 a10 = 0.0f, R32 a11 = 1.0f, R32 a12 = 0.0f,
          R32 a20 = 0.0f, R32 a21 = 0.0f, R32 a22 = 1.0f)
  {
    Data[0][0] = a00; Data[0][1] = a01; Data[0][2] = a02;
    Data[1][0] = a10; Data[1][1] = a11; Data[1][2] = a12;
    Data[2][0] = a20; Data[2][1] = a21; Data[2][2] = a22;
  }

  Matrix3(const Matrix4& other);


  static Matrix3 ToMatrix3(const Matrix4& other);

  R32         determinant() const;
  Matrix3     transpose() const;
  Matrix3     inverse() const;
  Matrix3     adjugate() const;
  
  Matrix3     operator*(const Matrix3& other) const;
  Matrix3     operator+(const Matrix3& other) const;
  Matrix3     operator-(const Matrix3& other) const;

  Matrix3     operator*(const R32 scaler) const;

  R32*        raw() { return Data[0]; }
  R32*        operator[](const size_t i);
  R32         get(const size_t row, const size_t col) const { return Data[row][col]; }
  R32         operator()(const size_t row, const size_t col) const { return get(row, col); }

  B8          operator==(const Matrix3& other) const;
  B8          operator!=(const Matrix3& other) const;
};

Log&          operator<<(Log& log, const Matrix3& mat3);
} // Recluse