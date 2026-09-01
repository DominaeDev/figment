#pragma once

#define _ENABLE_EXTENDED_ALIGNED_STORAGE // Resolves error C2338 on VS2026

#include "Figment.h"
#include "text/Context.h"
#include "app/AppSettings.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "chat/ChatStaging.h"

#include "llm/LLMTypes.h"
#include "llm/LLMStatus.h"

#define USE_WIN32_API 0 // Use Win32 calls for file i/o

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>

	// Undefine troublesome macros
	#undef LoadImage
	#undef DrawText
	#undef SetCursor
	
	#define PLATFORM_WINDOWS 1

	constexpr bool WindowsBuild = true;
#else
	#undef PLATFORM_WINDOWS
	constexpr bool WindowsBuild = false;
#endif