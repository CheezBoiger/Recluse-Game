// Copyright (c) Recluse Project. All rights reserved.
#pragma once


#if defined(_WIN32)
 #define _CRT_SECURE_NO_WARNINGS 1
 #include <Windows.h>
 #include <windowsx.h>
 #define RECLUSE_OS_WIN32      "win32"
#else
 #error "This Engine only works on Win32 systems! No other operating system is supported!"
#endif