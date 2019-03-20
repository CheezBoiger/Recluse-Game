// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Window.hpp"
#include "Win32/Keyboard.hpp"
#include "Win32/Mouse.hpp"
#include "Logging/Log.hpp"
#include "Exception.hpp"

#define RECLUSE_WINDOW_CLASS_NAME   L"RecluseWin32Window"
#define RECLUSE_WINDOW_PROP_NAME    L"RecluseWin32Prop"

namespace Recluse {


b32                        Window::bInitialized = false;
WindowResizeCallback      Window::gWindowResizeCallback = nullptr;
KeyboardCallback          Window::gKeyboardCallback = nullptr;
MouseButtonCallback       Window::gMouseButtonCallback = nullptr;
MousePositionCallback     Window::gMousePositionCallback = nullptr;
WindowInactiveCallack     Window::gWindowInactiveCallback = nullptr;
u32                       Window::kFullscreenHeight = 0;
u32                       Window::kFullscreenWidth = 0;
BYTE                      lpb[1 << 23];


b32 Window::isInitialized()
{
  return bInitialized;
}


void Window::setWindowResizeCallback(WindowResizeCallback callback)
{
  gWindowResizeCallback = callback;
}


void Window::setKeyboardCallback(KeyboardCallback callback)
{
  gKeyboardCallback = callback;
}


void Window::setMouseButtonCallback(MouseButtonCallback callback)
{
  gMouseButtonCallback = callback;
}


void Window::setMousePositionCallback(MousePositionCallback callback)
{
  gMousePositionCallback = callback;
}


void Window::SetWindowInactiveCallback(WindowInactiveCallack callback)
{
  gWindowInactiveCallback = callback;
}

b8    gFullScreenAltTab = false;
b8    gHooked = false;
HHOOK gFullScreenHook = NULL;


LRESULT CALLBACK LowLevelKeyboardProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
  // By returning a non-zero value from the hook procedure, the
  // message does not get passed to the target window
  KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *)lParam;
  BOOL bControlKeyDown = 0;
  gFullScreenAltTab = false;

  switch (nCode) {
  case HC_ACTION:
  {
    // Notify ALT+TAB signal.
    if (pkbhs->vkCode == VK_TAB && pkbhs->flags & LLKHF_ALTDOWN)
    { 
      gFullScreenAltTab = true;
      return 0;
    }
    break;
  }
  default:
    break;
  } 
  return CallNextHookEx(gFullScreenHook, nCode, wParam, lParam);
}


static void AdjustClientViewRect(HWND handle)
{
  if (!handle || !Mouse::isClamped()) return;
  
  RECT rect;
  GetClientRect(handle, &rect);
  ClientToScreen(handle, (POINT*)&rect.left);
  ClientToScreen(handle, (POINT*)&rect.right);
  ClipCursor(&rect);
}


LRESULT CALLBACK Window::windowProc(HWND   hwnd,
  UINT   uMsg, WPARAM wParam, LPARAM lParam)
{
  Window* window = reinterpret_cast<Window*>(GetPropW(hwnd, RECLUSE_WINDOW_PROP_NAME));
  if (gFullScreenAltTab && window->isFullScreen() && !window->isMinimized()) {
    window->setToWindowed(window->getWidth(), window->getHeight());
    window->minimize();
  }

  switch (uMsg) {
  case WM_DESTROY:
  {
    if (!window->mHandle) break;
    // Window on destroy.
    window->mHandle = NULL;
  } break;
  case WM_QUIT:
  case WM_CLOSE:
  {
    if (!window->mHandle) break;
    window->close();
  } break;
  case WM_SYSKEYDOWN:
  case WM_KEYDOWN:
  {

#define CHECK_KEY_STATE_DOWN(keyCode) { \
    SHORT s = GetKeyState(keyCode); \
    if (s & 0x8000) gKeyboardCallback(window, i32(keyCode), MapVirtualKey(u32(keyCode), MAPVK_VK_TO_VSC), WM_KEYDOWN, 0); \
  }
#define CHECK_KEY_STATE_UP(keyCode) { \
    SHORT s = GetKeyState(keyCode); \
    if (~(s & 0x8000)) gKeyboardCallback(window, i32(keyCode), MapVirtualKey(u32(keyCode), MAPVK_VK_TO_VSC), WM_KEYUP, 0); \
  }

    if (gKeyboardCallback) { 
      gKeyboardCallback(window, i32(wParam), MapVirtualKey(u32(wParam), MAPVK_VK_TO_VSC), WM_KEYDOWN, 0); 
      if (wParam == KEY_CODE_SHIFT) {
        CHECK_KEY_STATE_DOWN(VK_LSHIFT);
        CHECK_KEY_STATE_DOWN(VK_RSHIFT);
      } else if (wParam == KEY_CODE_CONTROL) {
        CHECK_KEY_STATE_DOWN(VK_LCONTROL);
        CHECK_KEY_STATE_DOWN(VK_RCONTROL);
      }
    }
  } break;
  case WM_SYSKEYUP:
  case WM_KEYUP:
  {
    if (gKeyboardCallback) {
      gKeyboardCallback(window, i32(wParam), MapVirtualKey(u32(wParam), MAPVK_VK_TO_VSC), WM_KEYUP, 0);
      if (wParam == KEY_CODE_SHIFT) {
        CHECK_KEY_STATE_UP(VK_LSHIFT);
        CHECK_KEY_STATE_UP(VK_RSHIFT);
      }else if (wParam == KEY_CODE_CONTROL) {
        CHECK_KEY_STATE_UP(VK_LCONTROL);
        CHECK_KEY_STATE_UP(VK_RCONTROL);
      }
    }
  } break;
  case WM_MOVE:
  {
    if (window) {
      window->m_PosX = (int)(short) LOWORD(lParam);
      window->m_PosY = (int)(short) HIWORD(lParam);
    }
  }  break;
  case WM_SIZE:
  {
    b32 windowChange = false;
    if (window) {
      i32 width = LOWORD(lParam);
      i32 height = HIWORD(lParam);
      if (width != window->mWidth || height != window->mHeight) {
        window->mWidth  = width;
        window->mHeight = height;
        windowChange = true;
      }

      switch (wParam) {
      case SIZE_RESTORED:
      {
        if (window->isMinimized() && gFullScreenAltTab && !window->isFullScreen()) {
          window->setToFullScreen();
          gFullScreenAltTab = false;
        }
        window->mMinimized = false;
      } break;
      case SIZE_MINIMIZED:
      {
        window->mMinimized = true;
      }
      }

      if (windowChange && gWindowResizeCallback) {
        gWindowResizeCallback(window, window->mWidth, window->mHeight);
      }
    }

  } break;
  case WM_SYSCOMMAND:
  {
  } break;
  case WM_MOUSEMOVE:
  {
    if (window && Mouse::isEnabled() && Mouse::isTracking()) {
      AdjustClientViewRect(window->mHandle);

      const int x = GET_X_LPARAM(lParam);
      const int y = GET_Y_LPARAM(lParam);
      window->inputMousePos(x, y);
      
    }
  } break;
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
  {
    if (window) {
      Mouse::ButtonType type;
      switch (uMsg) {
      case WM_LBUTTONDOWN: type = Mouse::LEFT; break;
      case WM_RBUTTONDOWN: type = Mouse::RIGHT; break;  
      default: type = Mouse::UNKNOWN; break;
      };

      Mouse::buttonActions[(i32)type] = Mouse::PRESSED;

      int X = (int)(short) GET_X_LPARAM(lParam);
      int Y = (int)(short) GET_Y_LPARAM(lParam);
      if (gMouseButtonCallback) gMouseButtonCallback(window, type, 
                                  Mouse::buttonActions[(i32)type], 0);
    }
  } break;
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  {
    if (window) {
      Mouse::ButtonType type;
      switch (uMsg) {
      case WM_LBUTTONUP: type = Mouse::LEFT; break;
      case WM_RBUTTONUP: type = Mouse::RIGHT; break;
      default: type = Mouse::UNKNOWN; break;
      };

      Mouse::buttonActions[(i32)type] = Mouse::RELEASED;

      int X = (int)(short)GET_X_LPARAM(lParam);
      int Y = (int)(short)GET_Y_LPARAM(lParam);
      if (gMouseButtonCallback) gMouseButtonCallback(window, type,
        Mouse::buttonActions[(i32)type], 0);
    }
  } break;
  case WM_INPUT:
  {
    if (window && !Mouse::isEnabled() && Mouse::isTracking()) {
      i32 dx, dy;
      UINT dwSize;

      AdjustClientViewRect(window->mHandle);

      if (!lpb) {
        break;
      }
      
      if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == ((UINT)-1)) {
        Log(rWarning) << "Raw input is not returning correct size.\n";
        break;
      }
      RAWINPUT* raw = (RAWINPUT*)lpb;
      if (raw->header.dwType == RIM_TYPEMOUSE) {
        if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
          dx = raw->data.mouse.lLastX - (i32)Mouse::lastXPos;
          dy = raw->data.mouse.lLastY - (i32)Mouse::lastYPos;
        } else {
          dx = raw->data.mouse.lLastX;
          dy = raw->data.mouse.lLastY;
        }
      }

      window->inputMousePos((i32)Mouse::xPos + dx, (i32)Mouse::yPos + dy);

      Mouse::lastXPos += (r64)dx;
      Mouse::lastYPos += (r64)dy;
    }
  } break;
  default: break;
  }

  return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}


b32 Window::initializeAPI()
{
  if (bInitialized) return true;

  WNDCLASSEXW winclass = { };
  winclass.cbSize = sizeof(WNDCLASSEXW);
  winclass.lpfnWndProc = windowProc;
  winclass.hInstance = GetModuleHandle(NULL);
  winclass.lpszClassName = RECLUSE_WINDOW_CLASS_NAME;
  winclass.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
  winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  winclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  
  if (!RegisterClassExW(&winclass)) return false;

  Mouse::cursor = GetCursor();
  bInitialized = true;

  // Set up window fullscreen constants.
  kFullscreenWidth = GetSystemMetrics(SM_CXSCREEN);
  kFullscreenHeight = GetSystemMetrics(SM_CYSCREEN);

  for (size_t i = 0; i < Mouse::MAX_MOUSE_BUTTONS; ++i) {
    Mouse::buttonActions[i] = Mouse::IDLE;
  }

  return bInitialized;
}


void Window::pollEvents()
{
  for (size_t i = 0; i < Mouse::MAX_MOUSE_BUTTONS; ++i) {
    if (Mouse::buttonActions[i] == Mouse::RELEASED) {
      Mouse::buttonActions[i] = Mouse::IDLE;
    }
  }

  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}


Window::~Window()
{
  if (mHandle != NULL && !shouldClose()) {
    close();
  }
}


b32 Window::create(std::string title, i32 width, i32 height)
{
  m_PosX = 0;
  m_PosY = 0;
  mWidth = width;
  mHeight = height;

  wchar_t* ltitle = nullptr;
  int size = 0;

  size = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), (int )title.size(), NULL, 0);

  ltitle = new wchar_t[size + 1];
  ltitle[size] = L'\0';

  MultiByteToWideChar(CP_UTF8, 0, title.c_str(), (int )title.size(), ltitle, size);

  mHandle = CreateWindowExW(NULL, RECLUSE_WINDOW_CLASS_NAME,
    ltitle, (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX), 
    m_PosX, m_PosY, mWidth, mHeight, NULL, NULL, GetModuleHandle(NULL), NULL);

  delete[] ltitle;

  if (!mHandle) {
    R_DEBUG(rError, "Failed to create window!\n");
    return false;
  }
  
  SetPropW(mHandle, RECLUSE_WINDOW_PROP_NAME, this);

  RECT windowRect = { 0, 0, mWidth, mHeight };
  AdjustWindowRect(&windowRect, WS_CAPTION, GetMenu(mHandle) != NULL);
  MoveWindow(mHandle, 0, 0,
    windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);

  // Register the raw input device, for when we disable the mouse cursor.
  RAWINPUTDEVICE rid = { 0x01, 0x02, 0, mHandle };
  if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
    Log(rError) << "Could not register mouse device!\n";
  }

  UpdateWindow(mHandle);
  return true;
}


void Window::show()
{
  ShowWindow(mHandle, SW_SHOW);
  mShowing = true;
}


void Window::hide()
{
  ShowWindow(mHandle, SW_HIDE);
  mShowing = false;
}


void Window::maximize()
{
  ShowWindow(mHandle, SW_MAXIMIZE);
  mShowing = true;
  mMinimized = false;
}


void Window::minimize()
{
  mShowing = false;
  mMinimized = true;
  ShowWindow(mHandle, SW_MINIMIZE);
}


void Window::setToCenter()
{
  RECT rect;
  GetWindowRect(mHandle, &rect);

  i32 xPos = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
  i32 yPos = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;

  SetWindowPos(mHandle, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  UpdateWindow(mHandle);
}


void Window::setToFullScreen()
{
  if (!mFullScreen) {
    MONITORINFO mi = { sizeof(mi) };
    HMONITOR monitor = MonitorFromWindow(mHandle, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(monitor, &mi);
    RECT& screen = mi.rcMonitor;

    // Set To fullscreen.
    SetWindowLongW(mHandle, GWL_EXSTYLE, WS_EX_TOPMOST);
    SetWindowLongW(mHandle, GWL_STYLE, WS_POPUP | WS_VISIBLE);

    SetWindowPos(mHandle, HWND_TOPMOST, 
      screen.left, 
      screen.top,   
      screen.right - screen.left, 
      screen.bottom - screen.top,
      SWP_FRAMECHANGED);
  
    if (!gHooked) {
      gFullScreenHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
      gHooked = true;
    }
  }

  UpdateWindow(mHandle);
  mFullScreen = true;
}


void Window::setToWindowed(i32 width, i32 height, b32 borderless)
{
  if (mFullScreen && gHooked) { 
    UnhookWindowsHookEx(gFullScreenHook);
    gHooked = false;
  }

  SetWindowLongW(mHandle, GWL_EXSTYLE, 0);
  SetWindowLongW(mHandle, GWL_STYLE, borderless ? (WS_POPUP | WS_VISIBLE) : (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX));
 
  SetWindowPos(mHandle, HWND_NOTOPMOST, 0, 0, width, height, (SWP_FRAMECHANGED));
  
  if (!borderless) {
    RECT windowRect = { 0, 0, width, height };
    AdjustWindowRect(&windowRect, WS_CAPTION, GetMenu(mHandle) != NULL);
    MoveWindow(mHandle, 0, 0, 
      windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);
  }

  UpdateWindow(mHandle);
  mFullScreen = false;
}


void Window::close()
{
  mRequestClose = true;

  if (mFullScreen && gHooked) {
    UnhookWindowsHookEx(gFullScreenHook);
    gHooked = false;
  }

  CloseWindow(mHandle);
  DestroyWindow(mHandle);
}


void Window::inputMousePos(i32 x, i32 y)
{
  if (Mouse::xPos == x && Mouse::yPos == y) return;

  Mouse::xPos = (r64)x;
  Mouse::yPos = (r64)y;

  if (gMousePositionCallback) {
    gMousePositionCallback(this, (r64)x, (r64)y);
  }
}


u32 Window::getRefreshRate()
{
  DEVMODEA lDevMode;
  EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &lDevMode);
  DWORD hertz = lDevMode.dmDisplayFrequency;
  return u32(hertz);
}
} // Recluse 