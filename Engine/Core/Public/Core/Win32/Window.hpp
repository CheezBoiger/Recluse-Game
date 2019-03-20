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
  static b32                    isInitialized();
  static b32                    initializeAPI();
  static void                   pollEvents();

  static void                   setWindowResizeCallback(WindowResizeCallback callback);
  static void                   setKeyboardCallback(KeyboardCallback callback);
  static void                   setMouseButtonCallback(MouseButtonCallback callback);
  static void                   setMousePositionCallback(MousePositionCallback callback);
  static void                   SetWindowInactiveCallback(WindowInactiveCallack callback);

  static u32                    getFullscreenHeight() { return kFullscreenHeight; }
  static u32                    getFullscreenWidth() { return kFullscreenWidth; }

  Window()
    : mHandle(NULL)
    , mHeight(0)
    , mWidth(0)
    , mRequestClose(false)
    , mFullScreen(false)
    , mMinimized(false)
    , mShowing(false) { }

  ~Window();

  b32            create(std::string title, i32 width, i32 height);

  void          show();
  void          hide();
  void          minimize();
  void          maximize();
  void          setToCenter();
  void          close();
  // Setting to full screen, or windowed, or window borderless, you MUST explicitly
  // call show() AFTER, in order to update the window.
  void          setToFullScreen();
  void          setToWindowed(i32 width, i32 height, b32 borderless = false);
  
  i32           getWidth() const { return mWidth; }
  i32           getHeight() const { return mHeight; }
  i32           getX() const { return m_PosX; }
  i32           getY() const { return m_PosY; }

  HWND          getHandle() { return mHandle; }

  // Get the refresh rate (in hertz (Hz)) of currently displaying window.
  static u32   getRefreshRate();

  b32           shouldClose() const { return mRequestClose; }
  b32           isFullScreen() const { return mFullScreen; }
  b32           isShowing() const { return mShowing; }
  b32           isMinimized() const { return mMinimized; }
private:
  void          inputMousePos(i32 x, i32 y);
  static LRESULT windowProc(HWND hwnd, UINT, WPARAM, LPARAM);

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