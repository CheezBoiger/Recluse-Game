// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Mouse.hpp"
#include "Win32/Win32Configs.hpp"


namespace Recluse {



b8 Mouse::mouseTrack = true;
b8 Mouse::mouseEnabled = true;
b8 Mouse::mouseShow = true;
r64 Mouse::XPos = 0;
r64 Mouse ::YPos = 0;
r64 Mouse::LastXPos = 0;
r64 Mouse::LastYPos = 0;
HCURSOR Mouse::cursor = NULL;


void Mouse::Enable(b8 enable)
{
  if (enable && Mouse::cursor) {
    SetCursor(Mouse::cursor);
  } else {
    Mouse::cursor = GetCursor();
    SetCursor(NULL);
  }

  Mouse::mouseEnabled = enable;
}


void Mouse::Show(b8 enable)
{
  if (enable && Mouse::cursor) {
    ShowCursor(true);
  } else {
    ShowCursor(false);
  }

  mouseShow = enable;
}


void Mouse::SetPosition(r64 x, r64 y)
{
  SetCursorPos((i32)x, (i32)y);
}
} // Recluse