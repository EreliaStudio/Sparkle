#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#else
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <cstdint>
using UINT = unsigned int;
using WPARAM = unsigned long;
using LPARAM = long;
using LRESULT = long;
using UINT_PTR = unsigned long;

#ifndef LOWORD
#define LOWORD(l)   ((unsigned short)((l) & 0xFFFF))
#endif
#ifndef HIWORD
#define HIWORD(l)   ((unsigned short)(((l) >> 16) & 0xFFFF))
#endif
#ifndef MAKELPARAM
#define MAKELPARAM(l, h) ((LPARAM)(((unsigned short)(l)) | ((unsigned long)((unsigned short)(h))) << 16))
#endif
#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wParam) HIWORD(wParam)
#endif
#endif
