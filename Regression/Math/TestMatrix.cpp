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
  Matrix4 multId = identity * mat;

  Log(rNotify) << "\n" << mat << "\n";
  Log() << "Identity: \n" << identity << "\n"; 
  Log() << "multId: \n" << multId << "\n";
  TASSERT_E(mat, identity);
  TASSERT_E(multId, identity);

  Log(rNotify) << "Test 1x4 * 4x4:\n";
  Matrix4 testMat(
    2.0f, 5.0f,   1.0,    -1.0,
    4.0f, 6.6f,   23.0f,  0.0f,
    0.0f, 30.0f, -1.0f,   5.0f,
    9.2f, 1.0f,   1.0f,   1.0f
  );

  Vector4 vec(4.0f, 2.0f, 1.0f, 1.0f);

  Vector4 ans = vec * testMat;
  //Vector4 sol = Vector4(18.0f, 52.2f, 64.0f, 40.8f);
  Vector4 sol = Vector4(25.2f, 64.2f, 50.0f, 2.0f);
  Log() << "ans: " << ans << "\nsol: " << sol << "\n";
  TASSERT_E(ans, sol);


  Matrix4 scaleTest = Matrix4::Scale(Matrix4::Identity(), Vector3(0.5f, 0.5f, 0.5f));
  Matrix4 scaleDst = Matrix4(
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  );
  Log() << "scaleTest: " << scaleTest << "\n"
        << "scaleDst: " << scaleDst << "\n";
  TASSERT_E(scaleTest, scaleDst);

  return true;
}
} // Test