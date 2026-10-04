#pragma once

#define _ENABLE_EXTENDED_ALIGNED_STORAGE // Resolves error C2338 on VS2026

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// Undefine troublesome windows macros
#undef LoadImage
#undef DrawText
#undef SetCursor
#undef ReadFile
#undef DeleteFile
#undef GetDateFormat
#undef GetTimeFormat
#undef DialogBox
#undef CreateDialog
#define PLATFORM_WINDOWS 1
#define USE_WIN32_API 0 // Use Win32 calls for file i/o

constexpr bool WindowsBuild = true;
#else
#undef PLATFORM_WINDOWS
constexpr bool WindowsBuild = false;
#define USE_WIN32_API 0
#endif

#include "Figment.h"