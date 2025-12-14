#ifndef GLOBAL_STRINGS_H__
#define GLOBAL_STRINGS_H__
#pragma once

#include "Types.h"

namespace GlobalStrings
{
	inline constexpr const_string ApplicationTitle = "Figment";

	namespace Status
	{
		inline constexpr const_string LoadingModel = 
			"Loading model...";
		inline constexpr const_string LoadingModelPercentFmt = 
			"Loading model... {0}%";
		inline constexpr const_string InitializingChat = 
			"Initializing chat...";
		inline constexpr const_string ChatInitialized = 
			"Chat initialized";
		inline constexpr const_string FailedToInitializeChat = 
			"Failed to initialize chat";
		inline constexpr const_string ModelLoaded = 
			"Model loaded";
		inline constexpr const_string ModelUnloaded = 
			"Model unloaded";
		inline constexpr const_string FailedToLoadModel = 
			"Failed to load model";
		inline constexpr const_string GeneratingResponse = 
			"Generating response...";
		inline constexpr const_string RebuildingContext = 
			"Rebuilding context...";
		inline constexpr const_string Ready = 
			"Ready";
	}

	namespace Errors
	{
		// ...
	}
}
#endif
