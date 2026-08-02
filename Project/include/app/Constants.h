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
		constexpr fig::coord WindowDefaultWidth = 1320;
		constexpr fig::coord WindowDefaultHeight = 900;
		constexpr fig::coord WindowMinWidth = 800;
		constexpr fig::coord WindowMinHeight = 600;

		constexpr double DefaultFontSize = 18.5;
		constexpr double StatusBarFontSize = 14.5;
		constexpr double CharacterNameFontSize = 12.0;
		constexpr double ChatMessageFontSize = 16.0;
		constexpr fig::coord ChatScrollWidth = 720;
		constexpr fig::coord ChatTextBoxWidth = 680;
		
		constexpr float MouseScrollSpeed = 200.0f;
		constexpr float MouseScrollSmoothing = 14.0f;
		
		constexpr int32_t ProfileImageWidth = 256;
		constexpr int32_t ProfileImageHeight = 256;

		namespace Cards
		{
			constexpr fig::coord SpacingX = 16;
			constexpr fig::coord SpacingY = 20;
			constexpr float ZoomSmoothing = 10.0f;
			constexpr float ZoomVerticalShift = 0.5f;

			namespace Full
			{
				constexpr fig::coord Width = 320;
				constexpr fig::coord Height = 412;
				constexpr fig::coord ZoomPixels = 18;
				constexpr fig::coord BorderOffset = 16;
				constexpr fig::coord TextY = 32;

				constexpr fig::coord InnerMargin = 12;
				constexpr fig::coord FooterHeight = 80;

				namespace Tags
				{
					constexpr fig::coord Margin = 10;
					constexpr fig::coord Spacing = 6;
					constexpr fig::coord InnerMargin = 8;
					constexpr fig::coord MinWidth = 36;
					constexpr fig::coord RowHeight = 32;
					constexpr fig::coord Top = 70;
					constexpr fig::coord MaxRows = 2;
				}
			}

			namespace Half
			{
				constexpr fig::coord Width = 240;
				constexpr fig::coord Height = 309;
				constexpr fig::coord ZoomPixels = 16;
				constexpr fig::coord BorderOffset = 12;

				constexpr fig::coord InnerMargin = 12;
				constexpr fig::coord TextY = 28;
				constexpr fig::coord FooterHeight = 60;
			}
		}

		constexpr fig::coord CardWidth = Cards::Full::Width;
		constexpr fig::coord CardHeight = Cards::Full::Height;
		constexpr fig::coord HalfCardWidth = Cards::Half::Width;
		constexpr fig::coord HalfCardHeight = Cards::Half::Height;

		namespace SidePanel
		{
			constexpr fig::coord HeaderHeight = 48;
			constexpr fig::coord FooterHeight = 80;
			constexpr fig::coord Width = 260;
		}

		namespace InfoPanel
		{
			constexpr fig::coord DefaultWidth = 340;
			constexpr std::array<fig::coord, 12> Widths { 260, 280, 300, 320, 340, 360, 380, 400, 420, 440, 460, 480, };
			constexpr fig::coord DefaultImageSize = 600;
		}

		namespace ChatList
		{
			constexpr fig::coord Width = 760;
		}

		namespace Chat
		{
			constexpr float ImageZoomFactor = 1.1f;
		}
	}

	namespace Chat
	{
		constexpr fig::const_string DialogueTag		= "dialogue";
		constexpr fig::const_string DialogueTagEnd	= "/dialogue";
		constexpr fig::const_string ActionTag		= "emote";
		constexpr fig::const_string ActionTagEnd	= "/emote";
		constexpr fig::const_string ThoughtTag		= "thinking";
		constexpr fig::const_string ThoughtTagEnd	= "/thinking";
		constexpr fig::const_string NarrationTag	= "narrator";
		constexpr fig::const_string NarrationTagEnd = "/narrator";
		constexpr fig::const_string DirectionTag	= "director";
		constexpr fig::const_string DirectionTagEnd = "/director";

		constexpr fig::const_string StateReportTag = "change";

		constexpr int DefaultNarratorCooldown = 5;

		namespace Names
		{
			constexpr fig::const_string System = "system";
			constexpr fig::const_string Narrator = "Narrator";
			constexpr fig::const_string Director = "Director";
			constexpr fig::const_string Unknown = "Unknown";
			constexpr fig::const_string User = "You";
		}

		constexpr fig::coord SmallPortraitWidth = 56;
		constexpr std::array<fig::const_string, 23> ReservedCharacterIDs { "user", "char", "current", "director", "narrator", "system", "char1", "char2", "char3", "char4", "char5", "char6", "char7", "char8", "char", "bot1", "bot2", "bot3", "bot4", "bot5", "bot6", "bot7", "bot8"};
	}

	namespace DefaultModelSettings
	{
		constexpr int32_t ContextSize = static_cast<int32_t>(fig::llm::ContextSize::_8K);
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
		constexpr fig::const_string Macros = "./resources/prompting/macros.xml";
		constexpr fig::const_string PromptScaffold = "./resources/prompting/chat.scaffold";
		constexpr fig::const_string DefaultScenario = "./resources/prompting/default_scenario.xml";
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
