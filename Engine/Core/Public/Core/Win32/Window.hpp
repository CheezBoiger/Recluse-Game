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
typedef void(*WindowInactiveCallack)(Window* window);


// Window Class. Uses Win32 API for window creation. Sadly, this is will
// make our game engine dependent on windows operating systems.
class Window {
  static b32                    bInitialized;
  static WindowResizeCallback   gWindowResizeCallback;
  static KeyboardCallback       gKeyboardCallback;
  static MouseButtonCallback    gMouseButtonCallback;
  static MousePositionCallback  gMousePositionCallback;
  static WindowInactiveCallack  gWindowInactiveCallback;
  static u32                    kFullscreenWidth;
  static u32                    kFullscreenHeight;

public:
  // Initialize the Window API before creating windows!
  static b32                    Initialized();
  static b32                    InitializeAPI();
  static void                   PollEvents();

  static void                   SetWindowResizeCallback(WindowResizeCallback callback);
  static void                   SetKeyboardCallback(KeyboardCallback callback);
  static void                   SetMouseButtonCallback(MouseButtonCallback callback);
  static void                   SetMousePositionCallback(MousePositionCallback callback);
  static void                   SetWindowInactiveCallback(WindowInactiveCallack callback);

  static u32                    FullscreenHeight() { return kFullscreenHeight; }
  static u32                    FullscreenWidth() { return kFullscreenWidth; }

  Window()
    : mHandle(NULL)
    , mHeight(0)
    , mWidth(0)
    , mRequestClose(false)
    , mFullScreen(false)
    , mMinimized(false)
    , mShowing(false) { }

  ~Window();

  b32            Create(std::string title, i32 width, i32 height);

  void          Show();
  void          Hide();
  void          Minimize();
  void          Maximize();
  void          SetToCenter();
  void          Close();
  // Setting to full screen, or windowed, or window borderless, you MUST explicitly
  // call Show() AFTER, in order to update the window.
  void          SetToFullScreen();
  void          SetToWindowed(i32 width, i32 height, b32 borderless = false);
  
  i32           Width() const { return mWidth; }
  i32           Height() const { return mHeight; }
  i32           X() const { return m_PosX; }
  i32           Y() const { return m_PosY; }

  HWND          Handle() { return mHandle; }

  // Get the refresh rate (in hertz (Hz)) of currently displaying window.
  static u32   GetRefreshRate();

  b32           ShouldClose() const { return mRequestClose; }
  b32           FullScreen() const { return mFullScreen; }
  b32           Showing() const { return mShowing; }
  b32           Minimized() const { return mMinimized; }
private:
  void          InputMousePos(i32 x, i32 y);
  static LRESULT WindowProc(HWND hwnd, UINT, WPARAM, LPARAM);

  HWND          mHandle;

  i32           mHeight;
  i32           mWidth;
  i32           m_PosX;
  i32           m_PosY;

  b32            mRequestClose : 1;
  b32            mFullScreen   : 1;
  b32            mShowing      : 1;
  b32            mMinimized    : 1;
};
} // Recluse