// Copyright (c) 2017 Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {



class Mouse {
public:
  static const I32 MAX_MOUSE_BUTTONS = 3;
 
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
  static void setEnable(B32 enable);

  //Track mouse.
  static void setTrack(B32 enable) { mouseTrack = enable; }
  static void show(B32 enable);
  static void clampWithinWindow(B32 enable) { mouseClamped = enable; }

  static B32 isEnabled() { return mouseEnabled; }
  static B32 isShowing() { return mouseShow; }
  static B32 isTracking() { return mouseTrack; }
  static B32 isClamped() { return mouseClamped; }
  static void setPosition(R64 x, R64 y);
  static void setCursorImage(const std::string& imgPath);

  static R64 getX() { return xPos; }
  static R64 getY() { return yPos; }
  static R64 lastX() { return lastXPos; }
  static R64 lastY() { return lastYPos; }

  static B32 buttonDown(ButtonType type);
  static B32 buttonUp(ButtonType type);

  static void resetButtonActions();

private:
  static B32       mouseClamped;
  static B32       mouseTrack;
  static B32       mouseEnabled;
  static B32       mouseShow;
  static R64      xPos;
  static R64      yPos;

  static R64      lastXPos;
  static R64      lastYPos;
  static HCURSOR  cursor;

  // TODO(): Other buttons on the mouse should be supported too...
  static ButtonAction      buttonActions   [MAX_MOUSE_BUTTONS];
  friend class Window;
};
} // Recluse