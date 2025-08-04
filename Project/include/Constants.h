#pragma once

#include "Types.h"

#define DEFAULT_MODEL_LOCATION "M:\\LLM\\default_model.gguf"
//#define DEFAULT_EMBEDDING_MODEL_LOCATION "M:\\Embedding\\nomic-embed-text-v1.Q6_K.gguf"
#define DEFAULT_EMBEDDING_MODEL_LOCATION "M:\\Embedding\\all-MiniLM-L6-v2-Q6_K.gguf"
#define NOMIC_EMBEDDING FALSE

namespace Constants
{
	extern const char* const AppTitle;
	extern const int WindowWidth;
	extern const int WindowHeight;

	extern const double DefaultFontSize;
	extern const double StatusBarFontSize;
	extern const double CharacterNameFontSize;
	extern const double ChatMessageFontSize;
	extern const int ChatScrollWidth;

	constexpr const std::string_view DialogueTag		= "talk";
	constexpr const std::string_view ActionTag			= "act";
	constexpr const std::string_view ThoughtTag			= "think";
	constexpr const std::string_view NarrationTag		= "narrator";
	constexpr const std::string_view DirectionTag		= "director";

	constexpr const std::string_view DialogueTagEnd		= "/talk";
	constexpr const std::string_view ActionTagEnd		= "/act";
	constexpr const std::string_view ThoughtTagEnd		= "/think";
	constexpr const std::string_view NarrationTagEnd	= "/narrator";
	constexpr const std::string_view DirectionTagEnd	= "/director";

	constexpr const int ContextSize						= 4096;
	constexpr const int MaxResponseLength				= 256;
	constexpr const float ContextWindowKeepRatio		= 0.75f;
}
