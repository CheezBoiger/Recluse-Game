// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"

#include "Core/Utility/Vector.hpp"

namespace Recluse {


enum Presentation {
  SINGLE_BUFFERING,
  DOUBLE_BUFFERING,
  TRIPLE_BUFFERING
};


class UserParams {
public:
  Presentation  presentMode;
  r32           lod;
};
} // Recluse