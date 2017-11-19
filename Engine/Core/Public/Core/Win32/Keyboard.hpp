// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Core/Types.hpp"
#include "Core/Win32/Win32Configs.hpp"

namespace Recluse {


enum KeyCode {
  KEY_CODE_ENTER = VK_RETURN,
  KEY_CODE_BACK = VK_BACK,
  KEY_CODE_TAB = VK_TAB,
  KEY_CODE_SHIFT = VK_SHIFT,
  KEY_CODE_CONTROL = VK_CONTROL,
  KEY_CODE_ALT = VK_MENU,
  KEY_CODE_PAUSE = VK_PAUSE,
  KEY_CODE_CAPITAL = VK_CAPITAL,
  KEY_CODE_ESCAPE = VK_ESCAPE,
  KEY_CODE_SPACE = VK_SPACE,
  KEY_CODE_PRIOR = VK_PRIOR,
  KEY_CODE_CLEAR = VK_CLEAR,
  KEY_CODE_NEXT = VK_NEXT,
  KEY_CODE_END = VK_END, 
  KEY_CODE_HOME = VK_HOME,
  KEY_CODE_LEFT_ARROW = VK_LEFT,
  KEY_CODE_RIGHT_ARROW = VK_RIGHT,
  KEY_CODE_UP_ARROW = VK_UP,
  KEY_CODE_DOWN_ARROW = VK_DOWN,
  KEY_CODE_INSERT = VK_INSERT,
  KEY_CODE_DELETE = VK_DELETE,
  KEY_CODE_0 = 0x30,
  KEY_CODE_1 = 0x31,
  KEY_CODE_2 = 0x32,
  KEY_CODE_3 = 0x33,
  KEY_CODE_4 = 0x34,
  KEY_CODE_5 = 0x35,
  KEY_CODE_6 = 0x36,
  KEY_CODE_7 = 0x37,
  KEY_CODE_8 = 0x38,
  KEY_CODE_9 = 0x39,
  KEY_CODE_A = 0x41,
  KEY_CODE_B = 0x42,
  KEY_CODE_C = 0x43,
  KEY_CODE_D = 0x44,
  KEY_CODE_E = 0x45,
  KEY_CODE_F = 0x46,
  KEY_CODE_G = 0x47,
  KEY_CODE_H = 0x48,
  KEY_CODE_I = 0x49,
  KEY_CODE_J = 0x4A,
  KEY_CODE_K = 0x4B,
  KEY_CODE_L = 0x4C,
  KEY_CODE_M = 0x4D,
  KEY_CODE_N = 0x4E,
  KEY_CODE_O = 0x4F,
  KEY_CODE_P = 0x50,
  KEY_CODE_Q = 0x51,
  KEY_CODE_R = 0x52,
  KEY_CODE_S = 0x53,
  KEY_CODE_T = 0x54,
  KEY_CODE_U = 0x55,
  KEY_CODE_V = 0x56,
  KEY_CODE_W = 0x57,
  KEY_CODE_X = 0x58,
  KEY_CODE_Y = 0x59,
  KEY_CODE_Z = 0x5A,
  KEYCODE_LSHIFT = VK_LSHIFT,
  KEYCODE_RSHIFT = VK_RSHIFT,
  KEYCODE_LCONTROL = VK_LCONTROL,
  KEYCODE_RCONTROL = VK_RCONTROL,
}; 



enum KeyAction {
  KEY_DOWN = WM_KEYDOWN,
  KEY_UP = WM_KEYUP
};


// Keyboard map. Used for mapping stuff.
class Keyboard {
public:
  static KeyAction keys[256];
  static b8 KeyPressed(KeyCode keycode);
  static b8 KeyReleased(KeyCode keycode);
};
} // Recluse