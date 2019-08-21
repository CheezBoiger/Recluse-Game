// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


class Cpu {
public:

  static B8   IsIntel();
  static B8   IsAmd();

  // Count number of flipped bits with SWAR.
  static I32 BitsFlippedCount(I32 v);

private:
  static B8   CheckVendorString(char* str);
};
} // Recluse