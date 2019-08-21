// Copyright (c) Recluse Project. All rights reserved.
#pragma once

#include "Win32Configs.hpp"
#include "Core/Types.hpp"

namespace Recluse {

class Window;

// APIs for handling window callbacks from the windowing system.
typedef void(*WindowResizeCallback)(Window* window, I32 width, I32 height);
typedef void(*KeyboardCallback)(Window* window, I32 key, I32 scanCode, I32 action, I32 mods);
typedef void(*MouseButtonCallback)(Window* window, I32  button, I32 action, I32 mods);
typedef void(*MousePositionCallback)(Window* window, R64 x, R64 y);
typedef void(*WindowInactiveCallack)(Window* window);


// Window Class. Uses Win32 API for window creation. Sadly, this is will
// make our game engine dependent on windows operating systems.
class Window {
  static B32                    bInitialized;
  static WindowResizeCallback   gWindowResizeCallback;
  static KeyboardCallback       gKeyboardCallback;
  static MouseButtonCallback    gMouseButtonCallback;
  static MousePositionCallback  gMousePositionCallback;
  static WindowInactiveCallack  gWindowInactiveCallback;
  static U32                    kFullscreenWidth;
  static U32                    kFullscreenHeight;

public:
  // Initialize the Window API before creating windows!
  static B32                    isInitialized();
  static B32                    initializeAPI();
  static void                   pollEvents();

  static void                   setWindowResizeCallback(WindowResizeCallback callback);
  static void                   setKeyboardCallback(KeyboardCallback callback);
  static void                   setMouseButtonCallback(MouseButtonCallback callback);
  static void                   setMousePositionCallback(MousePositionCallback callback);
  static void                   SetWindowInactiveCallback(WindowInactiveCallack callback);

  static U32                    getFullscreenHeight() { return kFullscreenHeight; }
  static U32                    getFullscreenWidth() { return kFullscreenWidth; }

  Window()
    : mHandle(NULL)
    , mHeight(0)
    , mWidth(0)
    , mRequestClose(false)
    , mFullScreen(false)
    , mMinimized(false)
    , mShowing(false) { }

  ~Window();

  B32            create(std::string title, I32 width, I32 height);

  void          show();
  void          hide();
  void          minimize();
  void          maximize();
  void          setToCenter();
  void          close();
  // Setting to full screen, or windowed, or window borderless, you MUST explicitly
  // call show() AFTER, in order to update the window.
  void          setToFullScreen();
  void          setToWindowed(I32 width, I32 height, B32 borderless = false);
  
  I32           getWidth() const { return mWidth; }
  I32           getHeight() const { return mHeight; }
  I32           getX() const { return m_PosX; }
  I32           getY() const { return m_PosY; }

  HWND          getHandle() { return mHandle; }

  // Get the refresh rate (in hertz (Hz)) of currently displaying window.
  static U32   getRefreshRate();

  B32           shouldClose() const { return mRequestClose; }
  B32           isFullScreen() const { return mFullScreen; }
  B32           isShowing() const { return mShowing; }
  B32           isMinimized() const { return mMinimized; }
private:
  void          inputMousePos(I32 x, I32 y);
  static LRESULT windowProc(HWND hwnd, UINT, WPARAM, LPARAM);

  HWND          mHandle;

  I32           mHeight;
  I32           mWidth;
  I32           m_PosX;
  I32           m_PosY;

  B32            mRequestClose : 1;
  B32            mFullScreen   : 1;
  B32            mShowing      : 1;
  B32            mMinimized    : 1;
};
} // Recluse