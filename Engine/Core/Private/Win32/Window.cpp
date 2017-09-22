// Copyright (c) 2017 Recluse Project. All rights reserved.
#include "Win32/Window.hpp"
#include "Win32/Keyboard.hpp"
#include "Win32/Mouse.hpp"

#include "Exception.hpp"


#define RECLUSE_WINDOW_CLASS_NAME   L"RecluseWin32Window"
#define RECLUSE_WINDOW_PROP_NAME    L"RecluseWin32Prop"

namespace Recluse {


b8                        Window::initialized = false;
WindowResizeCallback      Window::gWindowResizeCallback = nullptr;
KeyboardCallback          Window::gKeyboardCallback = nullptr;
MouseButtonCallback       Window::gMouseButtonCallback = nullptr;
MousePositionCallback     Window::gMousePositionCallback = nullptr;

b8 Window::Initialized()
{
  return initialized;
}


void Window::SetWindowResizeCallback(WindowResizeCallback callback)
{
  gWindowResizeCallback = callback;
}


void Window::SetKeyboardCallback(KeyboardCallback callback)
{
  gKeyboardCallback = callback;
}


void Window::SetMouseButtonCallback(MouseButtonCallback callback)
{
  gMouseButtonCallback = callback;
}


void Window::SetMousePositionCallback(MousePositionCallback callback)
{
  gMousePositionCallback = callback;
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


LRESULT CALLBACK Window::WindowProc(HWND   hwnd,
  UINT   uMsg, WPARAM wParam, LPARAM lParam)
{
  Window* window = reinterpret_cast<Window*>(GetPropW(hwnd, RECLUSE_WINDOW_PROP_NAME));
  if (window && window->mRequestClose) {
     uMsg = WM_CLOSE;
  }

  if (gFullScreenAltTab && window->FullScreen() && !window->Minimized()) {
    window->SetToWindowed(window->Width(), window->Height());
    window->Minimize();
  }

  switch (uMsg) {
  case WM_CLOSE:
  {
    window->Close();
    CloseWindow(window->Handle());
    DestroyWindow(window->Handle());
  } break;
  case WM_SYSKEYDOWN:
  case WM_KEYDOWN:
  {
    if (gKeyboardCallback) gKeyboardCallback(window, i32(wParam), MapVirtualKey(u32(wParam), MAPVK_VK_TO_VSC), WM_KEYDOWN, 0);
  } break;
  case WM_SYSKEYUP:
  case WM_KEYUP:
  {
    if (gKeyboardCallback) gKeyboardCallback(window, i32(wParam), MapVirtualKey(u32(wParam), MAPVK_VK_TO_VSC), WM_KEYUP, 0);
  } break;
  case WM_SIZE:
  {
    if (window) {
      window->mWidth  = LOWORD(lParam);
      window->mHeight = HIWORD(lParam);
    } 

    switch (wParam) {
      case SIZE_RESTORED:
      {
        if (window->Minimized() && gFullScreenAltTab && !window->FullScreen()) {
          window->SetToFullScreen();
          gFullScreenAltTab = false;
        }
        window->mMinimized = false;
      } break;
      case SIZE_MINIMIZED:
      {
        window->mMinimized = true;
      }
    }
    
    if (gWindowResizeCallback) {
      gWindowResizeCallback(window, window->mWidth, window->mHeight);
    }

  } break;
  case WM_SYSCOMMAND:
  {
  } break;
  case WM_MOUSEMOVE:
  {
    POINT point;
    if (window && GetCursorPos(&point)) {
      ScreenToClient(window->mHandle, &point);

      if (gMousePositionCallback) gMousePositionCallback(window, r64(point.x), r64(point.y));
    }
  } break;
  case WM_LBUTTONDOWN:
  {
  } break;
  case WM_LBUTTONUP:
  {
  } break;
  case WM_RBUTTONDOWN:
  {
  } break;
  case WM_RBUTTONUP:
  {
  } break;
  default: break;
  }

  return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}


b8 Window::InitializeAPI()
{
  if (initialized) return true;

  WNDCLASSEXW winclass = { };
  winclass.cbSize = sizeof(WNDCLASSEXW);
  winclass.lpfnWndProc = WindowProc;
  winclass.hInstance = GetModuleHandle(NULL);
  winclass.lpszClassName = RECLUSE_WINDOW_CLASS_NAME;
  winclass.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
  winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
  winclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  
  if (!RegisterClassExW(&winclass)) return false;

  initialized = true;
  return initialized;
}


void Window::PollEvents()
{
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);

    if (msg.message == WM_QUIT) {
      
    }
  }
}


Window::~Window()
{
  if (mHandle != NULL && !ShouldClose()) {
    Close();
  }
}


b8 Window::Create(std::string title, i32 width, i32 height)
{
  i32 xPos = 0;
  i32 yPos = 0;
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
    xPos, yPos, mWidth, mHeight, NULL, NULL, GetModuleHandle(NULL), NULL);

  if (!mHandle) {
    R_DEBUG("ERROR: Failed to create window!\n");
    return false;
  }
  
  SetPropW(mHandle, RECLUSE_WINDOW_PROP_NAME, this);

  RECT windowRect = { 0, 0, mWidth, mHeight };
  AdjustWindowRect(&windowRect, WS_CAPTION, GetMenu(mHandle) != NULL);
  MoveWindow(mHandle, 0, 0,
    windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, FALSE);

  UpdateWindow(mHandle);
  return true;
}


void Window::Show()
{
  ShowWindow(mHandle, SW_SHOW);
  mShowing = true;
}


void Window::Hide()
{
  ShowWindow(mHandle, SW_HIDE);
  mShowing = false;
}


void Window::Maximize()
{
  ShowWindow(mHandle, SW_MAXIMIZE);
  mShowing = true;
  mMinimized = false;
}


void Window::Minimize()
{
  mShowing = false;
  mMinimized = true;
  ShowWindow(mHandle, SW_MINIMIZE);
}


void Window::SetToCenter()
{
  RECT rect;
  GetWindowRect(mHandle, &rect);

  i32 xPos = (GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2;
  i32 yPos = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2;

  SetWindowPos(mHandle, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
  UpdateWindow(mHandle);
}


void Window::SetToFullScreen()
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


void Window::SetToWindowed(i32 width, i32 height, b8 borderless)
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


void Window::Close()
{
  mRequestClose = true;

  if (mFullScreen && gHooked) {
    UnhookWindowsHookEx(gFullScreenHook);
    gHooked = false;
  }
}
} // Recluse 