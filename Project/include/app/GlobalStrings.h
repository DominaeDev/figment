#pragma once

#include "Figment.h"

namespace fig::strings
{
	namespace Status
	{
		constexpr fig::const_string LoadingModel = 
			"Loading model\u2026";
		constexpr fig::const_string LoadingModelPercentFmt = 
			"Loading model\u2026 {0}%";
		constexpr fig::const_string InitializingChat = 
			"Initializing chat\u2026";
		constexpr fig::const_string ChatInitialized = 
			"Chat initialized";
		constexpr fig::const_string FailedToInitializeChat = 
			"Failed to initialize chat";
		constexpr fig::const_string ModelLoaded = 
			"Model loaded successfully";
		constexpr fig::const_string ModelUnloaded = 
			"Model unloaded";
		constexpr fig::const_string FailedToLoadModel = 
			"Failed to load model";
		constexpr fig::const_string GeneratingResponse = 
			"Generating response\u2026";
		constexpr fig::const_string RebuildingContext = 
			"Rebuilding context\u2026";
		constexpr fig::const_string Ready = 
			"Ready";
	}

	namespace User
	{
		constexpr fig::const_string DefaultUser =
			"Default User";
	}

	namespace LoadModelWidget
	{
		constexpr fig::const_string ModelLoading =
			"Loading\u2026";
		constexpr fig::const_string ModelLoaded =
			"Ready";
		constexpr fig::const_string ModelUnloaded =
			"Not ready";
		constexpr fig::const_string ModelError =
			"Load error";
	}

	namespace UI
	{
		constexpr fig::const_string MenuRecentChats =
			"Chats";
		constexpr fig::const_string MenuCharacters =
			"Characters";
		constexpr fig::const_string MenuScenarios =
			"Stories";
		constexpr fig::const_string MenuWorlds =
			"Worlds";
		constexpr fig::const_string New =
			"New";
	}

	namespace TTS
	{
		constexpr fig::const_string Generating =
			"Generating voice sample\u2026";
		constexpr fig::const_string ServerInitializing =
			"Text-to-speech server is starting up\u2026";
		constexpr fig::const_string ServerUnavailable =
			"Text-to-speech server is unavailable";
		constexpr fig::const_string LoadingModel =
			"Loading voice model\u2026";
		constexpr fig::const_string ErrorOccurred =
			"An error occurred";
		constexpr fig::const_string PlayingSound =
			"Playing\u2026";
	}

	namespace Error
	{
		constexpr const_string FileErrorMessage = 
			"File error (0x{:02X})";
		constexpr const_string FileAccessErrorMessage = 
			"File access denied";
		constexpr const_string DiskFullErrorMessage = 
			"Disk is full";
		constexpr const_string VerificationErrorMessage = 
			"Verification failed";
		constexpr const_string DownloadErrorMessage = 
			"Download failed";
	}

	namespace ProfileSettings
	{
		constexpr const_string RecoveryCodeHint =
			"In the event that you forget your password, you can use a recovery code to reset it. Make sure you write it down and store it securely. When a new recovery code is generated, any prior code will be rendered invalid.";
		constexpr const_string ConfirmationMessage = 
			"Are you really sure you want to delete this profile and all associated data?\nThis action cannot be undone.";
		constexpr const_string ConfirmationFailedError =
			"Password confirmation does not match.\nPlease try again.";
		constexpr const_string IncorrectPasswordError =
			"Incorrect password.\nPlease try again.";
		constexpr const_string PasswordChanged =
			"Password has been successfully changed.";
	}
}