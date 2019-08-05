// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Mouse.hpp"
#include "Win32/Win32Configs.hpp"


namespace Recluse {


b32 Mouse::mouseTrack = true;
b32 Mouse::mouseEnabled = true;
b32 Mouse::mouseShow = true;
b32 Mouse::mouseClamped = false;
r64 Mouse::xPos = 0;
r64 Mouse ::yPos = 0;
r64 Mouse::lastXPos = 0;
r64 Mouse::lastYPos = 0;
HCURSOR Mouse::cursor = NULL;
b32 Mouse::buttonActions[Mouse::MAX_MOUSE_BUTTONS];


void Mouse::setEnable(b32 enable) {
  if (enable && Mouse::cursor) {
    SetCursor(Mouse::cursor);
  } else {
    SetCursor(NULL);
  }

  Mouse::mouseEnabled = enable;
}


void Mouse::show(b32 enable)
{
  if (enable) {
    ShowCursor(true);
  } else {
    ShowCursor(false);
  }

  mouseShow = enable;
}


void Mouse::setPosition(r64 x, r64 y)
{
  SetCursorPos((i32)x, (i32)y);
  Mouse::xPos = x;
  Mouse::yPos = y;
  Mouse::lastXPos = x;
  Mouse::lastYPos = y;
}


b32 Mouse::buttonDown(Mouse::ButtonType type)
{
  return buttonActions[(i32)type] == Mouse::PRESSED;
}


b32 Mouse::buttonUp(Mouse::ButtonType type)
{
  return buttonActions[(i32)type] == Mouse::RELEASED;
}
} // Recluse