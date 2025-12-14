#ifndef GLOBAL_STRINGS_H__
#define GLOBAL_STRINGS_H__
#pragma once

#include <string_view>

namespace GlobalStrings
{
	inline constexpr fig::string_view ApplicationTitle = "Figment";

	namespace Status
	{
		inline constexpr fig::string_view LoadingModel = 
			"Loading model...";
		inline constexpr fig::string_view LoadingModelPercentFmt = 
			"Loading model... {0}%";
		inline constexpr fig::string_view InitializingChat = 
			"Initializing chat...";
		inline constexpr fig::string_view ChatInitialized = 
			"Chat initialized";
		inline constexpr fig::string_view FailedToInitializeChat = 
			"Failed to initialize chat";
		inline constexpr fig::string_view ModelLoaded = 
			"Model loaded";
		inline constexpr fig::string_view ModelUnloaded = 
			"Model unloaded";
		inline constexpr fig::string_view FailedToLoadModel = 
			"Failed to load model";
		inline constexpr fig::string_view GeneratingResponse = 
			"Generating response...";
		inline constexpr fig::string_view RebuildingContext = 
			"Rebuilding context...";
		inline constexpr fig::string_view Ready = 
			"Ready";
	}

	namespace Errors
	{
		// ...
	}
}
#endif
