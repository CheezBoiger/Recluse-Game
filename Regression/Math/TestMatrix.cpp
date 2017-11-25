// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TestMath.hpp"
#include "../Tester.hpp"
#include "Core/Math/Matrix4.hpp"


namespace Test {


b8 BasicMatrixMath()
{
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

  return true;
}
} // Test