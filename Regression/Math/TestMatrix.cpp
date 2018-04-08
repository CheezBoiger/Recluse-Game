// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TestMath.hpp"
#include "../Tester.hpp"
#include "Core/Math/Matrix4.hpp"


namespace Test {


b8 BasicMatrixMath()
{
  Log() << "\n\nMatrix Math\n\n";

  Matrix4 mat(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  );

  Matrix4 identity = Matrix4::Identity();

  Log(rNotify) << "\n" << mat << "\n";
  Log() << "Identity: \n" << identity << "\n"; 
  TASSERT_E(mat, identity);

  Log(rNotify) << "Test 4x4 * 4x1:\n";
  Matrix4 testMat(
    2.0f, 5.0f,   1.0,    -1.0,
    4.0f, 6.6f,   23.0f,  0.0f,
    0.0f, 30.0f, -1.0f,   5.0f,
    9.2f, 1.0f,   1.0f,   1.0f
  );

  Vector4 vec(4.0f, 2.0f, 1.0f, 1.0f);

  Vector4 ans = testMat * vec;
  Vector4 sol = Vector4(18.0f, 52.2f, 64.0f, 40.8f);
  Log() << "ans: " << ans << "\nsol: " << sol << "\n";
  TASSERT_E(ans, sol);

  return true;
}
} // Test