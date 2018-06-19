// Copyright (c) 2018 Recluse Project. All rights reserved.
#pragma once

#include "Vector3.hpp"
#include "Matrix4.hpp"


namespace Recluse {



// Oriented bounding box implementation.
struct OBB {
  

  Vector3     position;

  // Local Space R.
  Vector3     localR;

  Vector3     axis;
};
} // Recluse