// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Mouse.hpp"
#include "Win32/Win32Configs.hpp"


namespace Recluse {


B32 Mouse::mouseTrack = true;
B32 Mouse::mouseEnabled = true;
B32 Mouse::mouseShow = true;
B32 Mouse::mouseClamped = false;
R64 Mouse::xPos = 0;
R64 Mouse ::yPos = 0;
R64 Mouse::lastXPos = 0;
R64 Mouse::lastYPos = 0;
HCURSOR Mouse::cursor = NULL;
Mouse::ButtonAction Mouse::buttonActions[Mouse::MAX_MOUSE_BUTTONS];


void Mouse::setEnable(B32 enable) {
  if (enable && Mouse::cursor) {
    SetCursor(Mouse::cursor);
  } else {
    SetCursor(NULL);
  }

  Mouse::mouseEnabled = enable;
}


void Mouse::show(B32 enable)
{
  if ( enable ) 
  {
    ShowCursor( true );
  } 
  else 
  {
    ShowCursor( false );
  }

  mouseShow = enable;
}


void Mouse::resetButtonActions()
{
  for (U32 i = 0; i < MAX_MOUSE_BUTTONS; ++i)
  {
    buttonActions[ i ] = Mouse::IDLE;
  }
}


void Mouse::setPosition(R64 x, R64 y)
{
  SetCursorPos((I32)x, (I32)y);
  Mouse::xPos = x;
  Mouse::yPos = y;
  Mouse::lastXPos = x;
  Mouse::lastYPos = y;
}


B32 Mouse::buttonDown(Mouse::ButtonType type)
{
  return buttonActions[(I32)type] == Mouse::PRESSED;
}


B32 Mouse::buttonUp(Mouse::ButtonType type)
{
  return buttonActions[(I32)type] == Mouse::RELEASED;
}
} // Recluse