// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Mouse.hpp"
#include "Win32/Win32Configs.hpp"


namespace Recluse {



b32 Mouse::mouseTrack = true;
b32 Mouse::mouseEnabled = true;
b32 Mouse::mouseShow = true;
b32 Mouse::mouseClamped = false;
r64 Mouse::XPos = 0;
r64 Mouse ::YPos = 0;
r64 Mouse::LastXPos = 0;
r64 Mouse::LastYPos = 0;
HCURSOR Mouse::cursor = NULL;
b32 Mouse::buttonActions[Mouse::MAX_MOUSE_BUTTONS];


void Mouse::Enable(b32 enable)
{
  if (enable && Mouse::cursor) {
    SetCursor(Mouse::cursor);
  } else {
    SetCursor(NULL);
  }

  Mouse::mouseEnabled = enable;
}


void Mouse::Show(b32 enable)
{
  if (enable) {
    ShowCursor(true);
  } else {
    ShowCursor(false);
  }

  mouseShow = enable;
}


void Mouse::SetPosition(r64 x, r64 y)
{
  SetCursorPos((i32)x, (i32)y);
  Mouse::XPos = x;
  Mouse::YPos = y;
  Mouse::LastXPos = x;
  Mouse::LastYPos = y;
}


b32 Mouse::ButtonDown(Mouse::ButtonType type)
{
  return buttonActions[(i32)type] == Mouse::PRESSED;
}


b32 Mouse::ButtonUp(Mouse::ButtonType type)
{
  return buttonActions[(i32)type] == Mouse::RELEASED;
}
} // Recluse