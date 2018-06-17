// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


class Cpu {
public:

  static b8   IsIntel();
  static b8   IsAmd();

  // Count number of flipped bits with SWAR.
  static i32 BitsFlippedCount(i32 v);

private:
  static b8   CheckVendorString(char* str);
};
} // Recluse