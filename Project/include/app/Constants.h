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

		namespace Cards
		{
			constexpr fig::gui::Coord SpacingX = 18;
			constexpr fig::gui::Coord SpacingY = 20;
			constexpr float ZoomSmoothing = 10.0f;
			constexpr float ZoomVerticalShift = 0.5f;

			namespace Full
			{
				constexpr fig::gui::Coord Width = 320;
				constexpr fig::gui::Coord Height = 412;
				constexpr fig::gui::Coord ZoomPixels = 18;
				constexpr fig::gui::Coord BorderOffset = 16;
				constexpr fig::gui::Coord TextY = 32;

				constexpr fig::gui::Coord InnerMargin = 12;
				constexpr fig::gui::Coord FooterHeight = 80;

				namespace Tags
				{
					constexpr fig::gui::Coord Margin = 10;
					constexpr fig::gui::Coord Spacing = 6;
					constexpr fig::gui::Coord InnerMargin = 8;
					constexpr fig::gui::Coord MinWidth = 36;
					constexpr fig::gui::Coord RowHeight = 32;
					constexpr fig::gui::Coord Top = 70;
					constexpr fig::gui::Coord MaxRows = 2;
				}
			}

			namespace Half
			{
				constexpr fig::gui::Coord Width = 240;
				constexpr fig::gui::Coord Height = 309;
				constexpr fig::gui::Coord ZoomPixels = 16;
				constexpr fig::gui::Coord BorderOffset = 12;

				constexpr fig::gui::Coord InnerMargin = 12;
				constexpr fig::gui::Coord TextY = 28;
				constexpr fig::gui::Coord FooterHeight = 60;
			}
		}

		constexpr fig::gui::Coord CardWidth = Cards::Full::Width;
		constexpr fig::gui::Coord CardHeight = Cards::Full::Height;
		constexpr fig::gui::Coord HalfCardWidth = Cards::Half::Width;
		constexpr fig::gui::Coord HalfCardHeight = Cards::Half::Height;

		namespace SidePanel
		{
			constexpr fig::gui::Coord HeaderHeight = 48;
			constexpr fig::gui::Coord FooterHeight = 80;
			constexpr fig::gui::Coord Width = 256;
		}

		namespace ChatList
		{
			constexpr fig::gui::Coord Width = 760;
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

		constexpr fig::gui::Coord SmallPortraitWidth = 56;
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
