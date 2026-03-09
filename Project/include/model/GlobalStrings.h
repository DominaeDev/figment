#ifndef GLOBAL_STRINGS_H__
#define GLOBAL_STRINGS_H__
#pragma once

#include "Types.h"

namespace fig::strings
{
	inline constexpr fig::const_string ApplicationTitle = "Figment";

	namespace Status
	{
		inline constexpr fig::const_string LoadingModel = 
			"Loading model...";
		inline constexpr fig::const_string LoadingModelPercentFmt = 
			"Loading model... {0}%";
		inline constexpr fig::const_string InitializingChat = 
			"Initializing chat...";
		inline constexpr fig::const_string ChatInitialized = 
			"Chat initialized";
		inline constexpr fig::const_string FailedToInitializeChat = 
			"Failed to initialize chat";
		inline constexpr fig::const_string ModelLoaded = 
			"Model loaded";
		inline constexpr fig::const_string ModelUnloaded = 
			"Model unloaded";
		inline constexpr fig::const_string FailedToLoadModel = 
			"Failed to load model";
		inline constexpr fig::const_string GeneratingResponse = 
			"Generating response...";
		inline constexpr fig::const_string RebuildingContext = 
			"Rebuilding context...";
		inline constexpr fig::const_string Ready = 
			"Ready";
	}

	namespace UserProfile
	{
		inline constexpr fig::const_string DefaultUser =
			"Default User";
	}

	namespace UI
	{
		inline constexpr fig::const_string MenuRecentChats =
			"Recent chats";
		inline constexpr fig::const_string MenuCharacters =
			"Characters";
		inline constexpr fig::const_string MenuScenarios =
			"Scenarios";
	}

	namespace Errors
	{
		// ...
	}
}
#endif
