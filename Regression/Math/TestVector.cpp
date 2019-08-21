// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "TestMath.hpp"
#include "Core/Math/Vector4.hpp"
#include "Core/Math/Vector2.hpp"
#include "Core/Math/Vector3.hpp"

using namespace Recluse;

namespace Test {


B8 ToleranceSuccess(const Vector3& a, const Vector3& b)
{
  if (a.x > b.x + CONST_TOLERANCE || a.x < b.x - CONST_TOLERANCE) {
    Log() << "Vector addition failed!\n";
    return false;
  }

  if (a.y > b.y + CONST_TOLERANCE || a.y < b.y - CONST_TOLERANCE) {
    Log() << "Vector addition failed!\n";
    return false;
  }

  if (a.z > b.z + CONST_TOLERANCE || a.z < b.z - CONST_TOLERANCE) {
    Log() << "Vector addition failed!\n";
    return false;
  }

  return true;
}



B8 BasicVectorMath()
{
  Log() << "Vector Math\n";

  Log() << "Vector3 addition\n";
  R32 ex = 4.4777f;
  R32 ey = 1.3003f;
  R32 ez = -132.222f;

  Vector3 a3(3.4444f,  2.3333f, -132.222f);
  Vector3 b3(1.0333f, -1.033f,   0.0f);

  Vector3 c3 = a3 + b3;  
  Log() << "Calculated Addition Vector: " << c3 << "\n";

  if (!ToleranceSuccess(c3, Vector3(ex, ey, ez))) {
    return false;
  }

  Log() << "Vector3 subtraction\n";
  R32 sx = -2.4111f;
  R32 sy = -3.3663f;
  R32 sz = 132.222f;
  Vector3 c4 = b3 - a3;
  Log() << "Calculated Subtraction Vector: " << c4 << "\n";
  if (!ToleranceSuccess(c4, Vector3(sx, sy, sz))) {
    return false;
  }
  return true;
}
} // Test