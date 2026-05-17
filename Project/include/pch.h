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

#include "Figment.h"
#include "gui/GUICommon.h"
#include "gui/Events.h"

#if defined(_WIN32)
	#define USE_WIN32_API 1 // Use native Win32 system calls
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