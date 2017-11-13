// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Mouse.hpp"


namespace Recluse {



b8 Mouse::mouseEnabled = true;
r64 Mouse::XPos = 0;
r64 Mouse ::YPos = 0;
r64 Mouse::LastXPos = 0;
r64 Mouse::LastYPos = 0;
HCURSOR Mouse::cursor = NULL;


void Mouse::EnableMouse(b8 enable)
{
  if (enable && Mouse::cursor) {
    SetCursor(Mouse::cursor);
    ShowCursor(true);
  } else {
    Mouse::cursor = GetCursor();
    ShowCursor(false);
    SetCursor(NULL);
  }

  Mouse::mouseEnabled = enable;
}
} // Recluse