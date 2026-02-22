#pragma once

#include "Types.h"
#include "model/ChatTypes.h"
#include "model/GlobalStrings.h"

namespace Constants
{
	namespace GUI
	{
		constexpr int32_t WindowWidth = 1320;
		constexpr int32_t WindowHeight = 900;

		constexpr double DefaultFontSize = 18.5;
		constexpr double StatusBarFontSize = 14.5;
		constexpr double CharacterNameFontSize = 12.0;
		constexpr double ChatMessageFontSize = 16.0; //15.5;
		constexpr int32_t ChatScrollWidth = 800;
		
		constexpr float MouseScrollSpeed = 80.0f;

		namespace HomeScreen
		{
			constexpr int32_t CardWidth = 320;
			constexpr int32_t CardHeight = 412;
			constexpr int32_t CardSpacingX = 18;
			constexpr int32_t CardSpacingY = 20;
		}

		namespace SidePanel
		{
			constexpr float Width = 240.0f;
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
	}

	// Sizes
	enum class ContextSize : int32_t
	{
		_2K  = 1024 * 2,
		_3K  = 1024 * 3,
		_4K  = 1024 * 4,
		_6K  = 1024 * 6,
		_8K  = 1024 * 8,
		_10K = 1024 * 10,
		_12K = 1024 * 12,
		_16K = 1024 * 16,
		_24K = 1024 * 24,
		_32K = 1024 * 32,
	};

	namespace Context
	{
		constexpr int32_t DefaultSize = static_cast<int32_t>(ContextSize::_3K);
		constexpr int32_t MaxResponseLength = 256;
		constexpr int32_t MicroBatchSize = 512;
		constexpr float WindowKeepRatio = 0.75f;
		constexpr int32_t MaxSequences = 4;
	}

	namespace Paths
	{
		constexpr fig::const_string ProfilesFolder = "./profiles";
		constexpr fig::const_string ProfilesFileName = "profiles";
		constexpr fig::const_string ProfilesFileExt = "";
		constexpr fig::const_string ProfileIndexFileName = "index";
		constexpr fig::const_string ProfileIndexFileExt = "";
		constexpr fig::const_string AssetFileExt = "";
	}

	constexpr fig::const_string DefaultModelLocation = "M:\\LLM\\default_model.gguf";
	
	namespace LLM
	{
		constexpr uint32_t RandomSeed = 0xFFFFFFFF;
		constexpr uint32_t DebugSeed = 0xA1B2C3D4;
	}

	namespace Embedding
	{
		constexpr fig::const_string DefaultModelLocation = "M:\\Embedding\\all-MiniLM-L6-v2-Q6_K.gguf";
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

	namespace CharacterProperties
	{
		constexpr const char* const Gender = "gender";

	}
}
