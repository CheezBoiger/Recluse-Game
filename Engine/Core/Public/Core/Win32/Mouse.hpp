// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {



class Mouse {
public:
  enum ButtonAction {
    PRESSED,
    HOLD,
    RELEASED
  };

  enum ButtonType {
    LEFT,
    RIGHT,
    MIDDLE
  };

  static void Enable(b8 enable);
  static void Track(b8 enable) { mouseTrack = enable ; }
  static void Show(b8 enable);
  static void ClampWithinWindow(b8 enable);

  static b8 Enabled() { return mouseEnabled; }
  static b8 Showing() { return mouseShow; }
  static b8 Tracking() { return mouseTrack; }
  static b8 Clamped() { return mouseClamped; }
  static void SetPosition(r64 x, r64 y);

  static r64 X() { return XPos; }
  static r64 Y() { return YPos; }
  static r64 LastX() { return LastXPos; }
  static r64 LastY() { return LastYPos; }

private:
  static b8       mouseClamped;
  static b8       mouseTrack;
  static b8       mouseEnabled;
  static b8       mouseShow;
  static r64      XPos;
  static r64      YPos;

  static r64      LastXPos;
  static r64      LastYPos;
  static HCURSOR  cursor;
  friend class Window;
};
} // Recluse