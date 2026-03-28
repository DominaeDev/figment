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

#define USE_WIN32_API 1

#if USE_WIN32_API
	#if defined(_WIN32)
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#undef min
		#undef max
		#undef LoadImage
		#undef DrawText
	#else
		#undef USE_WIN32_API
	#endif
#endif