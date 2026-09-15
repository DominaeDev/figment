#pragma once

#include "Figment.h"

namespace fig::strings
{
	inline constexpr fig::const_string ApplicationTitle = "Figment";

	namespace Status
	{
		inline constexpr fig::const_string LoadingModel = 
			"Loading model\u2026";
		inline constexpr fig::const_string LoadingModelPercentFmt = 
			"Loading model\u2026 {0}%";
		inline constexpr fig::const_string InitializingChat = 
			"Initializing chat\u2026";
		inline constexpr fig::const_string ChatInitialized = 
			"Chat initialized";
		inline constexpr fig::const_string FailedToInitializeChat = 
			"Failed to initialize chat";
		inline constexpr fig::const_string ModelLoaded = 
			"Model loaded successfully";
		inline constexpr fig::const_string ModelUnloaded = 
			"Model unloaded";
		inline constexpr fig::const_string FailedToLoadModel = 
			"Failed to load model";
		inline constexpr fig::const_string GeneratingResponse = 
			"Generating response\u2026";
		inline constexpr fig::const_string RebuildingContext = 
			"Rebuilding context\u2026";
		inline constexpr fig::const_string Ready = 
			"Ready";
	}

	namespace UserProfile
	{
		inline constexpr fig::const_string DefaultUser =
			"Default User";
	}

	namespace LoadModelWidget
	{
		inline constexpr fig::const_string ModelLoading =
			"Loading\u2026";
		inline constexpr fig::const_string ModelLoaded =
			"Ready";
		inline constexpr fig::const_string ModelUnloaded =
			"Not ready";
		inline constexpr fig::const_string ModelError =
			"Load error";
	}

	namespace UI
	{
		inline constexpr fig::const_string MenuRecentChats =
			"Chats";
		inline constexpr fig::const_string MenuCharacters =
			"Characters";
		inline constexpr fig::const_string MenuScenarios =
			"Stories";
		inline constexpr fig::const_string MenuWorlds =
			"Worlds";
		inline constexpr fig::const_string New =
			"New";
	}

	namespace TTS
	{
		inline constexpr fig::const_string Generating =
			"Generating voice sample\u2026";
		inline constexpr fig::const_string ServerInitializing =
			"Text-to-speech server is starting up\u2026";
		inline constexpr fig::const_string ServerUnavailable =
			"Text-to-speech server is unavailable";
		inline constexpr fig::const_string LoadingModel =
			"Loading voice model\u2026";
		inline constexpr fig::const_string ErrorOccurred =
			"An error occurred";
		inline constexpr fig::const_string PlayingSound =
			"Playing\u2026";
	}

	namespace Errors
	{
		// ...
	}
}