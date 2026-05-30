#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"
#include "chat/ChatTypes.h"
#include "chat/ChatOptions.h"
#include "llm/ContextSize.h"

namespace fig::Constants
{
	namespace GUI
	{
		constexpr fig::gui::Coord WindowDefaultWidth = 1320;
		constexpr fig::gui::Coord WindowDefaultHeight = 900;
		constexpr fig::gui::Coord WindowMinWidth = 800;
		constexpr fig::gui::Coord WindowMinHeight = 600;

		constexpr double DefaultFontSize = 18.5;
		constexpr double StatusBarFontSize = 14.5;
		constexpr double CharacterNameFontSize = 12.0;
		constexpr double ChatMessageFontSize = 16.0;
		constexpr fig::gui::Coord ChatScrollWidth = 800;
		constexpr fig::gui::Coord ChatTextBoxWidth = 680;
		
		constexpr float MouseScrollSpeed = 200.0f;
		constexpr float MouseScrollSmoothing = 14.0f;
		
		constexpr int32_t ProfileImageWidth = 256;
		constexpr int32_t ProfileImageHeight = 256;

		constexpr fig::gui::Coord CardWidth = 320;
		constexpr fig::gui::Coord CardHeight = 412;
		constexpr fig::gui::Coord CardSpacingX = 18;
		constexpr fig::gui::Coord CardSpacingY = 20;
		
		constexpr fig::gui::Coord CardZoomPixelsLarge = 18;
		constexpr fig::gui::Coord CardZoomPixelsSmall = 16;

		constexpr float HalfScaleFactor = 0.75f;
		constexpr fig::gui::Coord HalfCardWidth = static_cast<fig::gui::Coord>(CardWidth * HalfScaleFactor);
		constexpr fig::gui::Coord HalfCardHeight = static_cast<fig::gui::Coord>(CardHeight * HalfScaleFactor);
		constexpr float CardZoomVerticalShift = 0.65f;

		namespace SidePanel
		{
			constexpr fig::gui::Coord HeaderHeight = 48;
			constexpr fig::gui::Coord FooterHeight = 80;
			constexpr fig::gui::Coord Width = 256;
		}
	}

	namespace Chat
	{
		constexpr fig::const_string DialogueTag = "talk";
		constexpr fig::const_string ActionTag = "act";
		constexpr fig::const_string ThoughtTag = "think";
		constexpr fig::const_string NarrationTag = "narrator";
		constexpr fig::const_string DirectionTag = "director";
		constexpr fig::const_string StateReportTag = "change";
		constexpr fig::const_string DialogueTagEnd = "/talk";
		constexpr fig::const_string ActionTagEnd = "/act";
		constexpr fig::const_string ThoughtTagEnd = "/think";
		constexpr fig::const_string NarrationTagEnd = "/narrator";
		constexpr fig::const_string DirectionTagEnd = "/director";

		constexpr int DefaultNarratorCooldown = 5;

		namespace Names
		{
			constexpr fig::const_string System = "system";
			constexpr fig::const_string Narrator = "Narrator";
			constexpr fig::const_string Director = "Director";
			constexpr fig::const_string Unknown = "Unknown";
		}

		constexpr fig::gui::Coord SmallPortraitWidth = 56;
	}

	namespace DefaultModelSettings
	{
		constexpr fig::llm::ContextSize ContextSize = fig::llm::ContextSize::_8K;
		constexpr float ContextWindowKeepRatio = 0.75f;

		constexpr int32_t GPULayers = 99;
		constexpr int32_t MaxResponseLength = 256;
		constexpr int32_t MicroBatchSize = 512;
		constexpr int32_t MaxSequences = 4;
		constexpr bool UseMLock = true;
		constexpr bool UseMMap = true;
	}

	namespace Paths
	{
		constexpr fig::const_string AppSettings = "./settings.ini";
		constexpr fig::const_string UserSettings = "user-settings.ini";
		constexpr fig::const_string ProfilesFolder = "./profiles";
		constexpr fig::const_string ProfilesFileName = "profiles";
		constexpr fig::const_string ProfilesFileExt = "";
		constexpr fig::const_string AssetIndexFileName = "index";
		constexpr fig::const_string AssetIndexFileExt = "";
		constexpr fig::const_string RecoveryFileName = "recovery";
		constexpr fig::const_string RecoveryFileExt = "";
		constexpr fig::const_string ProfileImageFileName = "image";
		constexpr fig::const_string ProfileImageFileExt = "";
		constexpr fig::const_string AssetFileExt = "";
	}
	
	namespace LLM
	{
		constexpr uint32_t RandomSeed = 0xFFFFFFFF;
		constexpr uint32_t DebugSeed = 0xA1B2C3D4;

		constexpr fig::chat::ChatOptions DefaultChatOptions {
			.flags {
				fig::chat::ChatOptions::Flag::GreetUser,
				fig::chat::ChatOptions::Flag::Uncensored,
		//		fig::chat::ChatOptions::Flag::LimitMessages,
		//		fig::chat::ChatOptions::Flag::RandomizeMessageCount,
		//		fig::chat::ChatOptions::Flag::StateVariables,
		//		fig::chat::ChatOptions::Flag::ReportStateChanges,
		//		fig::chat::ChatOptions::Flag::Embeddings,
				},
			.groupChatMode = fig::chat::ChatOptions::GroupChatMode::SwapSequences,
		};
	}

	namespace Embedding
	{
		constexpr fig::const_string EmbeddingSaveLocation = ".\\embeddings\\";
		constexpr int32_t ContextSize = 384;
		constexpr int32_t Depth = 1;
		constexpr bool SplitSentences = true;
		constexpr fig::const_string DocumentPrefix	= "";
		constexpr fig::const_string QueryPrefix		= "";
//		constexpr fig::const_string DocumentPrefix	= "search_document: ";
//		constexpr fig::const_string QueryPrefix		= "search_query: ";
//		constexpr fig::const_string DocumentPrefix	= "Represent the statement: ";
//		constexpr fig::const_string QueryPrefix		= "Represent the statement: ";
	}

	namespace CharacterAttributes
	{
		const fig::const_string Age = "age";
		const fig::const_string Appearance = "appearance";
		const fig::const_string Background = "background";
		const fig::const_string Persona = "persona";
		const fig::const_string Personality = "personality";
	}

	namespace Data
	{
		constexpr int32_t SmallPortraitWidth = 128;
	}
}
