// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"


namespace Recluse {


class Cpu {
public:

  static b8   IsIntel();
  static b8   IsAmd();

private:
  static b8   CheckVendorString(char* str);
};
} // Recluse