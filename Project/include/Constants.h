#pragma once

#include "model/ChatTypes.h"
#include "model/GlobalStrings.h"

namespace Constants
{
	namespace GUI
	{
		inline constexpr int WindowWidth = 1280;
		inline constexpr int WindowHeight = 900;

		inline constexpr double DefaultFontSize = 18.5;
		inline constexpr double StatusBarFontSize = 14.5;
		inline constexpr double CharacterNameFontSize = 12.0;
		inline constexpr double ChatMessageFontSize = 16.0; //15.5;
		inline constexpr int ChatScrollWidth = 800;
	}

	namespace Chat
	{
		inline constexpr std::string_view DialogueTag = "talk";
		inline constexpr std::string_view ActionTag = "act";
		inline constexpr std::string_view ThoughtTag = "think";
		inline constexpr std::string_view NarrationTag = "narrator";
		inline constexpr std::string_view DirectionTag = "director";
		inline constexpr std::string_view StateReportTag = "change";
		inline constexpr std::string_view DialogueTagEnd = "/talk";
		inline constexpr std::string_view ActionTagEnd = "/act";
		inline constexpr std::string_view ThoughtTagEnd = "/think";
		inline constexpr std::string_view NarrationTagEnd = "/narrator";
		inline constexpr std::string_view DirectionTagEnd = "/director";

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

	inline constexpr std::string_view DefaultModelLocation = "M:\\LLM\\default_model.gguf";
	
	namespace LLM
	{
		inline constexpr int32_t DefaultSeed = 0xFFFFFFFF;
		inline constexpr int32_t DebugSeed = 0xA1B2C3D4;
	}

	namespace Embedding
	{
		inline constexpr std::string_view DefaultModelLocation = "M:\\Embedding\\all-MiniLM-L6-v2-Q6_K.gguf";
		inline constexpr std::string_view EmbeddingSaveLocation = ".\\embeddings\\";
		inline constexpr int32_t ContextSize = 384;
		inline constexpr int32_t Depth = 1;
		inline constexpr bool SplitSentences = true;
		inline constexpr std::string_view DocumentPrefix	= "";
		inline constexpr std::string_view QueryPrefix		= "";
//		inline constexpr std::string_view DocumentPrefix	= "search_document: ";
//		inline constexpr std::string_view QueryPrefix		= "search_query: ";
//		inline constexpr std::string_view DocumentPrefix	= "Represent the statement: ";
//		inline constexpr std::string_view QueryPrefix		= "Represent the statement: ";
	}
}
