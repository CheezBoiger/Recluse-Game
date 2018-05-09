// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {



class Mouse {
public:
  static const i32 MAX_MOUSE_BUTTONS = 3;
 
  enum ButtonAction {
    IDLE,
    PRESSED,
    HOLD,
    RELEASED
  };

  enum ButtonType {
    UNKNOWN = -1,
    LEFT = 0,
    RIGHT = 1,
    MIDDLE = 2,
  };

  static void Enable(b32 enable);
  static void Track(b32 enable) { mouseTrack = enable; }
  static void Show(b32 enable);
  static void ClampWithinWindow(b32 enable) { mouseClamped = enable; }

  static b32 Enabled() { return mouseEnabled; }
  static b32 Showing() { return mouseShow; }
  static b32 Tracking() { return mouseTrack; }
  static b32 Clamped() { return mouseClamped; }
  static void SetPosition(r64 x, r64 y);

  static r64 X() { return XPos; }
  static r64 Y() { return YPos; }
  static r64 LastX() { return LastXPos; }
  static r64 LastY() { return LastYPos; }

  static b32 ButtonDown(ButtonType type);
  static b32 ButtonUp(ButtonType type);

private:
  static b32       mouseClamped;
  static b32       mouseTrack;
  static b32       mouseEnabled;
  static b32       mouseShow;
  static r64      XPos;
  static r64      YPos;

  static r64      LastXPos;
  static r64      LastYPos;
  static HCURSOR  cursor;

  // TODO(): Other buttons on the mouse should be supported too...
  static b32      buttonActions   [MAX_MOUSE_BUTTONS];
  friend class Window;
};
} // Recluse