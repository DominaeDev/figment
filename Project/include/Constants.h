#pragma once

#include "Types.h"
#include "llm/LLMTypes.h"

namespace Constants
{
	inline constexpr const char* AppTitle = "Figment Chat";

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

		inline constexpr int DefaultNarratorCooldownLength = 5;
	}

	namespace Context
	{
		inline constexpr int Size = 4096; //32768
		inline constexpr int MaxResponseLength = 256;
		inline constexpr int MicroBatchSize = 512;
		inline constexpr float WindowKeepRatio = 0.75f;
		inline constexpr int MaxSequences = 4;

		static constexpr std::array<SequenceId, 4> AllSequenceIDs {
			SequenceId::Bot1,
			SequenceId::Bot2,
			SequenceId::Bot3,
			SequenceId::Bot4,
		};
	}

	inline constexpr std::string_view DefaultModelLocation = "M:\\LLM\\default_model.gguf";
	
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
