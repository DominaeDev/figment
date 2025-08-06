#pragma once

#include "Types.h"

namespace Constants
{
	inline constexpr const char* AppTitle = "Llama chat";
	inline constexpr int WindowWidth = 1280;
	inline constexpr int WindowHeight = 900;

	inline constexpr double DefaultFontSize = 18.5;
	inline constexpr double StatusBarFontSize = 14.5;
	inline constexpr double CharacterNameFontSize = 12.0;
	inline constexpr double ChatMessageFontSize = 16.0; //15.5;
	inline constexpr int ChatScrollWidth = 800;

	inline constexpr std::string_view DialogueTag		= "talk";
	inline constexpr std::string_view ActionTag			= "act";
	inline constexpr std::string_view ThoughtTag		= "think";
	inline constexpr std::string_view NarrationTag		= "narrator";
	inline constexpr std::string_view DirectionTag		= "director";

	inline constexpr std::string_view DialogueTagEnd	= "/talk";
	inline constexpr std::string_view ActionTagEnd		= "/act";
	inline constexpr std::string_view ThoughtTagEnd		= "/think";
	inline constexpr std::string_view NarrationTagEnd	= "/narrator";
	inline constexpr std::string_view DirectionTagEnd	= "/director";

	inline constexpr int ContextSize					= 4096;
	inline constexpr int MaxResponseLength				= 256;
	inline constexpr float ContextWindowKeepRatio		= 0.75f;

	inline constexpr std::string_view DefaultModelLocation = "M:\\LLM\\default_model.gguf";
	inline constexpr std::string_view DefaultEmbeddingModelLocation = "M:\\Embedding\\all-MiniLM-L6-v2-Q6_K.gguf";
//	inline constexpr std::string_view DefaultEmbeddingModelLocation = "M:\\Embedding\\gist-all-minilm-l6-v2.Q8_0.gguf";

}
