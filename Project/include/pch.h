#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <ranges>
#include <optional>
#include <algorithm>
#include <codecvt>
#include <cwctype>
#include <stack>
#include <queue>
#include <expected>

#include "Types.h"
#include "Constants.h"
#include "util/Common.h"
#include "gui/GUICommon.h"

#if defined(_WIN32)
	#define USE_WIN32_API 0 // Use native Win32 system calls
#endif

#if USE_WIN32_API
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>

	// Undefine troublesome macros
	#undef min
	#undef max
	#undef LoadImage
	#undef DrawText
#endif