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

  // Enable mouse on screen position. Setting to false, will use raw input.
  static void setEnable(b32 enable);

  //Track mouse.
  static void setTrack(b32 enable) { mouseTrack = enable; }
  static void show(b32 enable);
  static void clampWithinWindow(b32 enable) { mouseClamped = enable; }

  static b32 isEnabled() { return mouseEnabled; }
  static b32 isShowing() { return mouseShow; }
  static b32 isTracking() { return mouseTrack; }
  static b32 isClamped() { return mouseClamped; }
  static void setPosition(r64 x, r64 y);
  static void setCursorImage(const std::string& imgPath);

  static r64 getX() { return xPos; }
  static r64 getY() { return yPos; }
  static r64 lastX() { return lastXPos; }
  static r64 lastY() { return lastYPos; }

  static b32 buttonDown(ButtonType type);
  static b32 buttonUp(ButtonType type);

  static void resetButtonActions();

private:
  static b32       mouseClamped;
  static b32       mouseTrack;
  static b32       mouseEnabled;
  static b32       mouseShow;
  static r64      xPos;
  static r64      yPos;

  static r64      lastXPos;
  static r64      lastYPos;
  static HCURSOR  cursor;

  // TODO(): Other buttons on the mouse should be supported too...
  static ButtonAction      buttonActions   [MAX_MOUSE_BUTTONS];
  friend class Window;
};
} // Recluse