// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Matrix3.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"

namespace Recluse {

class Log;

// All matrices are row major, which is implemented by DirectX. Vulkan 
// Attempts to be essentially the same...
// Keep in mind, this is a row-major matrix, its format is as follows:
// 
//          Matrix[row][col]
// 
// 
// Applications that use this matrix must define their operations in row major order, especially
// with multiplication:
// if A multiply to B:      B*A = C
struct Matrix4 {
  R32                       Data[4][4];

  static Matrix4 identity() {
    return Matrix4();
  }

  // Transformations of our matrix.
  static Matrix4            rotate(const Matrix4& begin, const R32 radians, const Vector3& axis);
  static Matrix4            translate(const Matrix4& mat, const Vector3& position);
  static Matrix4            scale(const Matrix4& mat, const Vector3& scale);

  Matrix4(R32 a00 = 1.0f, R32 a01 = 0.0f, R32 a02 = 0.0f, R32 a03 = 0.0f,
          R32 a10 = 0.0f, R32 a11 = 1.0f, R32 a12 = 0.0f, R32 a13 = 0.0f,
          R32 a20 = 0.0f, R32 a21 = 0.0f, R32 a22 = 1.0f, R32 a23 = 0.0f,
          R32 a30 = 0.0f, R32 a31 = 0.0f, R32 a32 = 0.0f, R32 a33 = 1.0f)
  {
    Data[0][0] = a00; Data[0][1] = a01; Data[0][2] = a02; Data[0][3] = a03;
    Data[1][0] = a10; Data[1][1] = a11; Data[1][2] = a12; Data[1][3] = a13;
    Data[2][0] = a20; Data[2][1] = a21; Data[2][2] = a22; Data[2][3] = a23;
    Data[3][0] = a30; Data[3][1] = a31; Data[3][2] = a32; Data[3][3] = a33;
  }

  Matrix4(const R32* rawDat)
    : Matrix4(rawDat[0],  rawDat[1],  rawDat[2],    rawDat[3],
              rawDat[4],  rawDat[5],  rawDat[6],    rawDat[7],
              rawDat[8],  rawDat[9],  rawDat[10],   rawDat[11],
              rawDat[12], rawDat[13], rawDat[14],   rawDat[15]) 
  { }

  Matrix4(const R64* rawDat)
    : Matrix4(static_cast<R32>(rawDat[0]),  static_cast<R32>(rawDat[1]),  static_cast<R32>(rawDat[2]),    static_cast<R32>(rawDat[3]),
              static_cast<R32>(rawDat[4]),  static_cast<R32>(rawDat[5]),  static_cast<R32>(rawDat[6]),    static_cast<R32>(rawDat[7]),
              static_cast<R32>(rawDat[8]),  static_cast<R32>(rawDat[9]),  static_cast<R32>(rawDat[10]),   static_cast<R32>(rawDat[11]),
              static_cast<R32>(rawDat[12]), static_cast<R32>(rawDat[13]), static_cast<R32>(rawDat[14]),   static_cast<R32>(rawDat[15])) 
  { }

  Matrix4(const Vector4& row1,
          const Vector4& row2 = Vector4(0.0f, 1.0f, 0.0f, 0.0f),
          const Vector4& row3 = Vector4(0.0f, 0.0f, 1.0f, 0.0f),
          const Vector4& row4 = Vector4(0.0f, 0.0f, 0.0f, 1.0f))
    : Matrix4(row1.x, row1.y, row1.z, row1.w,
              row2.x, row2.y, row2.z, row2.w,
              row3.x, row3.y, row3.z, row3.w,
              row4.x, row4.y, row4.z, row4.w)
  { }

  // Retrieve the Perspective matrix, which is in left hand coordinates.
  static Matrix4          perspective(R32 fovy, R32 aspect, R32 zNear, R32 zFar);

  // Left hand Orthographic matrix.
  static Matrix4          ortho(R32 width, R32 height, R32 zNear, R32 zFar);

  // Retrieve the LookAt matrix, which is in left hand coordinates.
  static Matrix4          lookAt(const Vector3& eye, const Vector3& center, const Vector3& up);

  Matrix4                 operator*(const Matrix4& other) const;
  Matrix4                 operator+(const Matrix4& other) const;
  Matrix4                 operator-(const Matrix4& other) const;

  Matrix4                 operator*(const R32 scaler) const;  

  void                    operator*=(const Matrix4& other);
  void                    operator+=(const Matrix4& other);
  void                    operator-=(const Matrix4& other);

  B8                      operator==(const Matrix4& other) const;
  B8                      operator!=(const Matrix4& other) const;

  R32                     determinant() const;
  Matrix4                 adjugate() const;
  Matrix4                 inverse() const;
  Matrix4                 transpose() const;
  Matrix3                 minor(U32 row, U32 col) const;

  R32*                    raw() { return Data[0]; }
  R32*                    operator[](const size_t i);
  R32                     get(const size_t row, const size_t col) const { return Data[row][col]; }
  R32                     operator()(const size_t row, const size_t col) const { return get(row, col); }
};


Log&                      operator<<(Log& log, const Matrix4& mat4);
} // Recluse