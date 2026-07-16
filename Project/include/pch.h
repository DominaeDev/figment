#pragma once

#include "Figment.h"
#include "text/Context.h"
#include "app/AppSettings.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "chat/ChatStaging.h"

#include "llm/LLMTypes.h"
#include "llm/LLMStatus.h"

#define USE_WIN32_API 0 // Use Win32 calls for file i/o

#if defined(_WIN32) && USE_WIN32_API
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>

	// Undefine troublesome macros
	#undef LoadImage
	#undef DrawText
#endif