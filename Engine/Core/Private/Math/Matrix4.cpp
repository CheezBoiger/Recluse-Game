// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Math/Matrix4.hpp"
#include "Logging/Log.hpp"
#include "Exception.hpp"

#include <iomanip>
#include <fstream>
#include <cmath>

#if defined _M_X64 && __USE_INTEL_INTRINSICS__
#define FAST_INTRINSICS 
#include <xmmintrin.h>
#endif 

// This define is supposed to give some leaway when comparing matrices
// with eachother. A sort of +/- tolerance.
#define KINDA_SMALL_NUMBER     0.001f
#define TOLERANCE              KINDA_SMALL_NUMBER


namespace Recluse {


r32* Matrix4::operator[](const size_t i)
{
  return Data[i];
}


Matrix4 Matrix4::operator*(const Matrix4& other) const 
{
#if !defined FAST_INTRINSICS
  r32 m00 = Data[0][0]; r32 om00 = other.Data[0][0];
  r32 m01 = Data[0][1]; r32 om01 = other.Data[0][1];
  r32 m02 = Data[0][2]; r32 om02 = other.Data[0][2];
  r32 m03 = Data[0][3]; r32 om03 = other.Data[0][3];
  r32 m10 = Data[1][0]; r32 om10 = other.Data[1][0];
  r32 m11 = Data[1][1]; r32 om11 = other.Data[1][1];
  r32 m12 = Data[1][2]; r32 om12 = other.Data[1][2];
  r32 m13 = Data[1][3]; r32 om13 = other.Data[1][3];
  r32 m20 = Data[2][0]; r32 om20 = other.Data[2][0];
  r32 m21 = Data[2][1]; r32 om21 = other.Data[2][1];
  r32 m22 = Data[2][2]; r32 om22 = other.Data[2][2];
  r32 m23 = Data[2][3]; r32 om23 = other.Data[2][3];
  r32 m30 = Data[3][0]; r32 om30 = other.Data[3][0];
  r32 m31 = Data[3][1]; r32 om31 = other.Data[3][1];
  r32 m32 = Data[3][2]; r32 om32 = other.Data[3][2];
  r32 m33 = Data[3][3]; r32 om33 = other.Data[3][3];
  return Matrix4(
    m00 * om00 + m01 * om10 + m02 * om20 + m03 * om30,
    m00 * om01 + m01 * om11 + m02 * om21 + m03 * om31,
    m00 * om02 + m01 * om12 + m02 * om22 + m03 * om32,
    m00 * om03 + m01 * om13 + m02 * om23 + m03 * om33,

    m10 * om00 + m11 * om10 + m12 * om20 + m13 * om30,
    m10 * om01 + m11 * om11 + m12 * om21 + m13 * om31,
    m10 * om02 + m11 * om12 + m12 * om22 + m13 * om32,
    m10 * om03 + m11 * om13 + m12 * om23 + m13 * om33,

    m20 * om00 + m21 * om10 + m22 * om20 + m23 * om30,
    m20 * om01 + m21 * om11 + m22 * om21 + m23 * om31,
    m20 * om02 + m21 * om12 + m22 * om22 + m23 * om32,
    m20 * om03 + m21 * om13 + m22 * om23 + m23 * om33,

    m30 * om00 + m31 * om10 + m32 * om20 + m33 * om30,
    m30 * om01 + m31 * om11 + m32 * om21 + m33 * om31,
    m30 * om02 + m31 * om12 + m32 * om22 + m33 * om32,
    m30 * om03 + m31 * om13 + m32 * om23 + m33 * om33
  );
#else
  // With 64 multiplications and 48 additions, we can reduce this down to 
  // 16 multiplications and 12 additions using intrinsics. This is useful
  // as we are updating matrices every frame, for any matrix that may need 
  // to be updated.
  Matrix4 ans;
  __m128 row1 = _mm_load_ps(&(*other.Data)[0]);
  __m128 row2 = _mm_load_ps(&(*other.Data)[4]);
  __m128 row3 = _mm_load_ps(&(*other.Data)[8]);
  __m128 row4 = _mm_load_ps(&(*other.Data)[12]);
  for (i32 i = 0; i < 4; ++i) {
    __m128 brod1 = _mm_set1_ps((*Data)[4 * i + 0]);
    __m128 brod2 = _mm_set1_ps((*Data)[4 * i + 1]);
    __m128 brod3 = _mm_set1_ps((*Data)[4 * i + 2]);
    __m128 brod4 = _mm_set1_ps((*Data)[4 * i + 3]);
    __m128 row = _mm_add_ps(
                  _mm_add_ps(
                    _mm_mul_ps(brod1, row1),
                    _mm_mul_ps(brod2, row2)),
                  _mm_add_ps(
                    _mm_mul_ps(brod3, row3),
                    _mm_mul_ps(brod4, row4)));
    _mm_store_ps(&(*ans.Data)[4 * i], row);
  }
  return ans;
#endif
}


Matrix4 Matrix4::operator+(const Matrix4& other) const
{
  return Matrix4(
    Data[0][0] + other.Data[0][0], Data[0][1] + other.Data[0][1], Data[0][2] + other.Data[0][2], Data[0][3] + other.Data[0][3],
    Data[1][0] + other.Data[1][0], Data[1][1] + other.Data[1][1], Data[1][2] + other.Data[1][2], Data[1][3] + other.Data[1][3],
    Data[2][0] + other.Data[2][0], Data[2][1] + other.Data[2][1], Data[2][2] + other.Data[2][2], Data[2][3] + other.Data[2][3],
    Data[3][0] + other.Data[3][0], Data[3][1] + other.Data[3][1], Data[3][2] + other.Data[3][2], Data[3][3] + other.Data[3][3]
  );
}


Matrix4 Matrix4::operator-(const Matrix4& other) const
{
  return Matrix4(
    Data[0][0] - other.Data[0][0], Data[0][1] - other.Data[0][1], Data[0][2] - other.Data[0][2], Data[0][3] - other.Data[0][3],
    Data[1][0] - other.Data[1][0], Data[1][1] - other.Data[1][1], Data[1][2] - other.Data[1][2], Data[1][3] - other.Data[1][3],
    Data[2][0] - other.Data[2][0], Data[2][1] - other.Data[2][1], Data[2][2] - other.Data[2][2], Data[2][3] - other.Data[2][3],
    Data[3][0] - other.Data[3][0], Data[3][1] - other.Data[3][1], Data[3][2] - other.Data[3][2], Data[3][3] - other.Data[3][3]
  );
}


Matrix4 Matrix4::operator*(const r32 scaler) const
{
  return Matrix4(
    Data[0][0] * scaler, Data[0][1] * scaler, Data[0][2] * scaler, Data[0][3] * scaler,
    Data[1][0] * scaler, Data[1][1] * scaler, Data[1][2] * scaler, Data[1][3] * scaler,
    Data[2][0] * scaler, Data[2][1] * scaler, Data[2][2] * scaler, Data[2][3] * scaler,
    Data[3][0] * scaler, Data[3][1] * scaler, Data[3][2] * scaler, Data[3][3] * scaler
  );
}


void Matrix4::operator*=(const Matrix4& other)
{
  Matrix4 ori = *this;
  Data[0][0] = ori[0][0] * other.Data[0][0] + ori[0][1] * other.Data[1][0] + ori[0][2] * other.Data[2][0] + ori[0][3] * other.Data[3][0];
  Data[0][1] = ori[0][0] * other.Data[0][1] + ori[0][1] * other.Data[1][1] + ori[0][2] * other.Data[2][1] + ori[0][3] * other.Data[3][1];
  Data[0][2] = ori[0][0] * other.Data[0][2] + ori[0][1] * other.Data[1][2] + ori[0][2] * other.Data[2][2] + ori[0][3] * other.Data[3][2];
  Data[0][3] = ori[0][0] * other.Data[0][3] + ori[0][1] * other.Data[1][3] + ori[0][2] * other.Data[2][3] + ori[0][3] * other.Data[3][3];

  Data[1][0] = ori[1][0] * other.Data[0][0] + ori[1][1] * other.Data[1][0] + ori[1][2] * other.Data[2][0] + ori[1][3] * other.Data[3][0];
  Data[1][1] = ori[1][0] * other.Data[0][1] + ori[1][1] * other.Data[1][1] + ori[1][2] * other.Data[2][1] + ori[1][3] * other.Data[3][1];
  Data[1][2] = ori[1][0] * other.Data[0][2] + ori[1][1] * other.Data[1][2] + ori[1][2] * other.Data[2][2] + ori[1][3] * other.Data[3][2];
  Data[1][3] = ori[1][0] * other.Data[0][3] + ori[1][1] * other.Data[1][3] + ori[1][2] * other.Data[2][3] + ori[1][3] * other.Data[3][3];

  Data[2][0] = ori[2][0] * other.Data[0][0] + ori[2][1] * other.Data[1][0] + ori[2][2] * other.Data[2][0] + ori[2][3] * other.Data[3][0];
  Data[2][1] = ori[2][0] * other.Data[0][1] + ori[2][1] * other.Data[1][1] + ori[2][2] * other.Data[2][1] + ori[2][3] * other.Data[3][1];
  Data[2][2] = ori[2][0] * other.Data[0][2] + ori[2][1] * other.Data[1][2] + ori[2][2] * other.Data[2][2] + ori[2][3] * other.Data[3][2];
  Data[2][3] = ori[2][0] * other.Data[0][3] + ori[2][1] * other.Data[1][3] + ori[2][2] * other.Data[2][3] + ori[2][3] * other.Data[3][3];

  Data[3][0] = ori[3][0] * other.Data[0][0] + ori[3][1] * other.Data[1][0] + ori[3][2] * other.Data[2][0] + ori[3][3] * other.Data[3][0];
  Data[3][1] = ori[3][0] * other.Data[0][1] + ori[3][1] * other.Data[1][1] + ori[3][2] * other.Data[2][1] + ori[3][3] * other.Data[3][1];
  Data[3][2] = ori[3][0] * other.Data[0][2] + ori[3][1] * other.Data[1][2] + ori[3][2] * other.Data[2][2] + ori[3][3] * other.Data[3][2];
  Data[3][3] = ori[3][0] * other.Data[0][3] + ori[3][1] * other.Data[1][3] + ori[3][2] * other.Data[2][3] + ori[3][3] * other.Data[3][3];
}


void Matrix4::operator+=(const Matrix4& other)
{
  Data[0][0] += other.Data[0][0]; Data[0][1] += other.Data[0][1]; Data[0][2] += other.Data[0][2]; Data[0][3] += other.Data[0][3];
  Data[1][0] += other.Data[1][0]; Data[1][1] += other.Data[1][1]; Data[1][2] += other.Data[1][2]; Data[1][3] += other.Data[1][3];
  Data[2][0] += other.Data[2][0]; Data[2][1] += other.Data[2][1]; Data[2][2] += other.Data[2][2]; Data[2][3] += other.Data[2][3];
  Data[3][0] += other.Data[3][0]; Data[3][1] += other.Data[3][1]; Data[3][2] += other.Data[3][2]; Data[3][3] += other.Data[3][3];
}


void Matrix4::operator-=(const Matrix4& other)
{
  Data[0][0] -= other.Data[0][0]; Data[0][1] -= other.Data[0][1]; Data[0][2] -= other.Data[0][2]; Data[0][3] -= other.Data[0][3];
  Data[1][0] -= other.Data[1][0]; Data[1][1] -= other.Data[1][1]; Data[1][2] -= other.Data[1][2]; Data[1][3] -= other.Data[1][3];
  Data[2][0] -= other.Data[2][0]; Data[2][1] -= other.Data[2][1]; Data[2][2] -= other.Data[2][2]; Data[2][3] -= other.Data[2][3];
  Data[3][0] -= other.Data[3][0]; Data[3][1] -= other.Data[3][1]; Data[3][2] -= other.Data[3][2]; Data[3][3] -= other.Data[3][3];
}


b8 Matrix4::operator==(const Matrix4& other) const
{
  for (u32 i = 0; i < 4; ++i) {
    for (u32 j = 0; j < 4; ++j) {
      if (Data[i][j] != other.Data[i][j]) {
        return false;
      }
    }
  }
  return true;
}


b8 Matrix4::operator!=(const Matrix4& other) const
{
  return !(*this == other);
}


r32 Matrix4::Determinant() const
{
  return  Data[0][0] * (Data[1][1] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) -
                        Data[1][2] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) +
                        Data[1][3] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1])
                    ) -
          Data[0][1] * (Data[1][0] * (Data[2][2] * Data[3][3] - Data[2][3] * Data[3][2]) -
                        Data[1][2] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) +
                        Data[1][3] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0])
                    ) +
          Data[0][2] * (Data[1][0] * (Data[2][1] * Data[3][3] - Data[2][3] * Data[3][1]) -
                        Data[1][1] * (Data[2][0] * Data[3][3] - Data[2][3] * Data[3][0]) +
                        Data[1][3] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0])
                    ) -
          Data[0][3] * (Data[1][0] * (Data[2][1] * Data[3][2] - Data[2][2] * Data[3][1]) -
                        Data[1][1] * (Data[2][0] * Data[3][2] - Data[2][2] * Data[3][0]) +
                        Data[1][2] * (Data[2][0] * Data[3][1] - Data[2][1] * Data[3][0]) );
}


Matrix4 Matrix4::Adjugate() const
{
  // Calculating our adjugate using the transpose of the cofactor of our
  // matrix.
  Matrix4 CofactorMatrix;
  r32 sign = 1.0f;
  for (u32 row = 0; row < 4; ++row) {
    sign = -sign;
    for (u32 col = 0; col < 4; ++col) {
      sign = -sign;
      CofactorMatrix[row][col] = Minor(row, col).Determinant() * sign;
    }
  }
  // Transpose this CofactorMatrix to get the adjugate.
  return CofactorMatrix.Transpose();
}


Matrix3 Matrix4::Minor(u32 row, u32 col) const
{
  Matrix3 minor;
  u32 r = 0, c;
  for (u32 i = 0; i < 4; ++i) {
    if (i == row) continue;
    c = 0;
    for (u32 j = 0; j < 4; ++j) {
      if (j == col) continue;
      minor[r][c] = Data[i][j];
      c++;
    }
    r++;
  }
  return minor;
}


Matrix4 Matrix4::Transpose() const
{
  return Matrix4(
    Data[0][0], Data[1][0], Data[2][0], Data[3][0],
    Data[0][1], Data[1][1], Data[2][1], Data[3][1],
    Data[0][2], Data[1][2], Data[2][2], Data[3][2],
    Data[0][3], Data[1][3], Data[2][3], Data[3][3]
  );
}


Matrix4 Matrix4::Inverse() const
{
  r32 detA = Determinant();
  if (detA == 0.0f) {
    return Matrix4::Identity();
  }
  Matrix4 inverse = Adjugate() * (1.0f / detA);
  return inverse;
}


Matrix4 Matrix4::Perspective(r32 fovy, r32 aspect, r32 zNear, r32 zFar)
{
  r32 tanHalfFov = tanf(fovy * 0.5f);
  Matrix4 perspective = Matrix4::Identity();
  perspective[3][3] = 0.0f;
  perspective[0][0] = 1.0f / (aspect * tanHalfFov);
  perspective[1][1] = -(1.0f / tanHalfFov);
  perspective[2][3] = 1.0f;
  
  perspective[2][2] = (zFar + zNear) / (zFar - zNear);
  perspective[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
  return perspective;
}


Matrix4 Matrix4::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up)
{
  Vector3 front((center - eye).Normalize());
  Vector3 right(up.Cross(front).Normalize());
  Vector3 u(front.Cross(right));
  return Matrix4(
    right.x,          u.x,          front.x,        0.0f,
    right.y,          u.y,          front.y,        0.0f,
    right.z,          u.z,          front.z,        0.0f,
   -right.Dot(eye),  -u.Dot(eye),  -front.Dot(eye), 1.0f
  );
}


Matrix4 Matrix4::Ortho(r32 width, r32 height, r32 zNear, r32 zFar)
{
  Matrix4 ortho = Matrix4::Identity();
  ortho[0][0] =  2.0f / width;
  ortho[1][1] = -2.0f / height;
  ortho[2][2] =  1.0f / (zFar - zNear);
  ortho[3][2] = zNear / (zNear - zFar);
  return ortho;
}


Matrix4 Matrix4::Translate(const Matrix4& mat, const Vector3& position)
{
  Matrix4 matrix = mat;
  matrix[3][0] += matrix[0][0] * position.x;
  matrix[3][1] += matrix[1][1] * position.y;
  matrix[3][2] += matrix[2][2] * position.z;
  return matrix;
}


Matrix4 Matrix4::Rotate(const Matrix4& begin, const r32 radians, const Vector3& ax)
{
  r32 oneMinusCosine = 1.0f - cosf(radians);
  r32 cosine = cosf(radians);
  r32 sine = sinf(radians);

  Vector3 axis = ax.Normalize();

  Matrix4 rotator(
    cosine + (axis.x * axis.x) * oneMinusCosine,      oneMinusCosine * axis.y * axis.x + axis.z * sine, axis.z * axis.x * oneMinusCosine - axis.y * sine, 0,
    axis.x * axis.y * oneMinusCosine - axis.z * sine, cosine + (axis.y * axis.y) * oneMinusCosine,      axis.z * axis.y * oneMinusCosine + axis.x * sine, 0,
    axis.x * axis.z * oneMinusCosine + axis.y * sine, axis.y * axis.z * oneMinusCosine - axis.x * sine, cosine + (axis.z * axis.z) * oneMinusCosine,      0,
    0,                                                0,                                                0,                                                1
  );

  return rotator * begin;
}


Matrix4 Matrix4::Scale(const Matrix4& begin, const Vector3& scale)
{
  Matrix4 scaler(
    scale.x,  0.0f,     0.0f,     0.0f,
    0.0f,     scale.y,  0.0f,     0.0f,
    0.0f,     0.0f,     scale.z,  0.0f,
    0.0f,     0.0f,     0.0f,     1.0f
  );
  return scaler * begin;
}


Log& operator<<(Log& log, const Matrix4& mat4)
{
  for (size_t row = 0; row < 4; ++row) {
    for (size_t col = 0; col < 4; ++col) {
      log << std::setw(15) << mat4(row, col);
    }
    log << "\n";
  }
  return log;
}


Vector4 Matrix4::operator*(const Vector4& other) const
{
  return Vector4(
    Data[0][0] * other.x + Data[1][0] * other.y + Data[2][0] * other.z + Data[3][0] * other.w,
    Data[0][1] * other.x + Data[1][1] * other.y + Data[2][1] * other.z + Data[3][1] * other.w,
    Data[0][2] * other.x + Data[1][2] * other.y + Data[2][2] * other.z + Data[3][2] * other.w,
    Data[0][3] * other.x + Data[1][3] * other.y + Data[2][3] * other.z + Data[3][3] * other.w
  );
}
} // Recluse