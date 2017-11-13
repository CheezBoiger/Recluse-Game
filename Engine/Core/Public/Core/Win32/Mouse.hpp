// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {



class Mouse {
public:
  static void EnableMouse(b8 enable);

  static b8 Enabled() { return mouseEnabled; }


  static r64      XPos;
  static r64      YPos;

  static r64      LastXPos;
  static r64      LastYPos;

private:
  static b8       mouseEnabled;
  static HCURSOR  cursor;
};
} // Recluse