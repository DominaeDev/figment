#pragma once

#include "Figment.h"

#define USE_WIN32_API 0 // Use Win32 calls for file i/o

#if defined(_WIN32) && USE_WIN32_API
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>

	// Undefine troublesome macros
	#undef LoadImage
	#undef DrawText
#endif