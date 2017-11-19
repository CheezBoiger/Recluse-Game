// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Keyboard.hpp"

namespace Recluse {


KeyAction Keyboard::keys[256];


b8 Keyboard::KeyPressed(KeyCode key)
{
  return Keyboard::keys[(i32)key] == KEY_DOWN ? true : false;
}


b8 Keyboard::KeyReleased(KeyCode key)
{
  return Keyboard::keys[(i32)key] == KEY_UP ? true : false;
}
} // Recluse