#ifndef GLOBAL_STRINGS_H__
#define GLOBAL_STRINGS_H__
#pragma once

#include <string_view>

namespace GlobalStrings
{
	inline constexpr std::string_view ApplicationTitle = "Figment";

	namespace Status
	{
		inline constexpr std::string_view LoadingModel = 
			"Loading model...";
		inline constexpr std::string_view LoadingModelPercentFmt = 
			"Loading model... {0}%";
		inline constexpr std::string_view InitializingChat = 
			"Initializing chat...";
		inline constexpr std::string_view ChatInitialized = 
			"Chat initialized";
		inline constexpr std::string_view FailedToInitializeChat = 
			"Failed to initialize chat";
		inline constexpr std::string_view ModelLoaded = 
			"Model loaded";
		inline constexpr std::string_view ModelUnloaded = 
			"Model unloaded";
		inline constexpr std::string_view FailedToLoadModel = 
			"Failed to load model";
		inline constexpr std::string_view GeneratingResponse = 
			"Generating response...";
		inline constexpr std::string_view RebuildingContext = 
			"Rebuilding context...";
		inline constexpr std::string_view Ready = 
			"Ready";
	}

	namespace Errors
	{
		// ...
	}
}
#endif
