#pragma once

#include "Types.h"
#include "model/ChatTypes.h"
#include "model/GlobalStrings.h"

namespace Constants
{
	namespace GUI
	{
		inline constexpr int32_t WindowWidth = 1280;
		inline constexpr int32_t WindowHeight = 900;

		inline constexpr double DefaultFontSize = 18.5;
		inline constexpr double StatusBarFontSize = 14.5;
		inline constexpr double CharacterNameFontSize = 12.0;
		inline constexpr double ChatMessageFontSize = 16.0; //15.5;
		inline constexpr int32_t ChatScrollWidth = 800;
	}

	namespace Chat
	{
		inline constexpr fig::const_string DialogueTag = "talk";
		inline constexpr fig::const_string ActionTag = "act";
		inline constexpr fig::const_string ThoughtTag = "think";
		inline constexpr fig::const_string NarrationTag = "narrator";
		inline constexpr fig::const_string DirectionTag = "director";
		inline constexpr fig::const_string StateReportTag = "change";
		inline constexpr fig::const_string DialogueTagEnd = "/talk";
		inline constexpr fig::const_string ActionTagEnd = "/act";
		inline constexpr fig::const_string ThoughtTagEnd = "/think";
		inline constexpr fig::const_string NarrationTagEnd = "/narrator";
		inline constexpr fig::const_string DirectionTagEnd = "/director";

		inline constexpr int DefaultNarratorCooldown = 5;
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
		inline constexpr int32_t DefaultSize = static_cast<int32_t>(ContextSize::_3K);
		inline constexpr int32_t MaxResponseLength = 256;
		inline constexpr int32_t MicroBatchSize = 512;
		inline constexpr float WindowKeepRatio = 0.75f;
		inline constexpr int32_t MaxSequences = 4;
	}

	namespace Paths
	{
		inline constexpr fig::const_string ProfilesFolder = "./profiles";
		inline constexpr fig::const_string ProfileIndexFileName = "index";
		inline constexpr fig::const_string ProfileIndexFileExt = "xml";
		inline constexpr fig::const_string AssetFileExt = "";
	}

	inline constexpr fig::const_string DefaultModelLocation = "M:\\LLM\\default_model.gguf";
	
	namespace LLM
	{
		inline constexpr uint32_t RandomSeed = 0xFFFFFFFF;
		inline constexpr uint32_t DebugSeed = 0xA1B2C3D4;
	}

	namespace Embedding
	{
		inline constexpr fig::const_string DefaultModelLocation = "M:\\Embedding\\all-MiniLM-L6-v2-Q6_K.gguf";
		inline constexpr fig::const_string EmbeddingSaveLocation = ".\\embeddings\\";
		inline constexpr int32_t ContextSize = 384;
		inline constexpr int32_t Depth = 1;
		inline constexpr bool SplitSentences = true;
		inline constexpr fig::const_string DocumentPrefix	= "";
		inline constexpr fig::const_string QueryPrefix		= "";
//		inline constexpr fig::const_string DocumentPrefix	= "search_document: ";
//		inline constexpr fig::const_string QueryPrefix		= "search_query: ";
//		inline constexpr fig::const_string DocumentPrefix	= "Represent the statement: ";
//		inline constexpr fig::const_string QueryPrefix		= "Represent the statement: ";
	}
}
