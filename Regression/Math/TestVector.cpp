// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TestMath.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Vector2.hpp"
#include "Core/Math/Vector3.hpp"

using namespace Recluse;

namespace Test {



b8 BasicVectorMath()
{
  Log() << "Vector Math\n";



  Log() << "Vector3 addition\n";
  r32 ex = 4.4777f;
  r32 ey = 1.3003f;
  r32 ez = -132.222f;

  Vector3 a3(3.4444f,  2.3333f, -132.222f);
  Vector3 b3(1.0333f, -1.033f,   0.0f);

  Vector3 c3 = a3 + b3;  
  Log() << "Calculated Addition Vector: " << c3 << "\n";

  if (c3.x > ex + CONST_TOLERANCE || c3.x < ex - CONST_TOLERANCE) {
    Log() << "Vector addition failed!\n";
    return false;
  }

  if (c3.y > ey + CONST_TOLERANCE || c3.y < ey - CONST_TOLERANCE) {
    Log() << "Vector addition failed!\n";
    return false;
  }

  if (c3.z > ez + CONST_TOLERANCE || c3.z < ez - CONST_TOLERANCE) {
    Log() << "Vector addition failed!\n";
    return false;
  }

  return true;
}
} // Test