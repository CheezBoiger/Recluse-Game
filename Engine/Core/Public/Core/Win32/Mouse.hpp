// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {



class Mouse {
public:
  static void EnableMouse(b8 enable);

  static b8 Enabled() { return mouseEnabled; }


  static u32      XPos;
  static u32      YPos;

  static u32      LastXPos;
  static u32      LastYPos;

private:
  static b8       mouseEnabled;
  static HCURSOR  cursor;
};
} // Recluse