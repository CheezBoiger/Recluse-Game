// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {

class Window;

// APIs for handling window callbacks from the windowing system.
typedef void(*WindowResizeCallback)(Window* window, i32 width, i32 height);
typedef void(*KeyboardCallback)(Window* window, i32 key, i32 scanCode, i32 action, i32 mods);
typedef void(*MouseButtonCallback)(Window* window, i32  button, i32 action, i32 mods);
typedef void(*MousePositionCallback)(Window* window, r64 x, r64 y);


// Window Class. Uses Win32 API for window creation. Sadly, this is will
// make our game engine dependent on windows operating systems.
class Window {
  static b8                     initialized;
  static WindowResizeCallback   gWindowResizeCallback;
  static KeyboardCallback       gKeyboardCallback;
  static MouseButtonCallback    gMouseButtonCallback;
  static MousePositionCallback  gMousePositionCallback;

public:
  // Initialize the Window API before creating windows!
  static b8                     Initialized();
  static b8                     InitializeAPI();
  static void                   PollEvents();

  static void                   SetWindowResizeCallback(WindowResizeCallback callback);
  static void                   SetKeyboardCallback(KeyboardCallback callback);
  static void                   SetMouseButtonCallback(MouseButtonCallback callback);
  static void                   SetMousePositionCallback(MousePositionCallback callback);

  Window()
    : mHandle(NULL)
    , mHeight(0)
    , mWidth(0)
    , mRequestClose(false)
    , mFullScreen(false)
    , mMinimized(false)
    , mShowing(false) { }

  ~Window();

  b8            Create(std::string title, i32 width, i32 height);

  void          Show();
  void          Hide();
  void          Minimize();
  void          Maximize();
  void          SetToCenter();
  void          Close();
  // Setting to full screen, or windowed, or window borderless, you MUST explicitly
  // call Show() AFTER, in order to update the window.
  void          SetToFullScreen();
  void          SetToWindowed(i32 width, i32 height, b8 borderless = false);
  
  i32           Width() const { return mWidth; }
  i32           Height() const { return mHeight; }

  HWND          Handle() { return mHandle; }

  b8            ShouldClose() const { return mRequestClose; }
  b8            FullScreen() const { return mFullScreen; }
  b8            Showing() const { return mShowing; }
  b8            Minimized() const { return mMinimized; }
private:

  static LRESULT WindowProc(HWND hwnd, UINT, WPARAM, LPARAM);

  HWND          mHandle;

  i32           mHeight;
  i32           mWidth;

  b8            mRequestClose : 1;
  b8            mFullScreen   : 1;
  b8            mShowing      : 1;
  b8            mMinimized    : 1;
};
} // Recluse