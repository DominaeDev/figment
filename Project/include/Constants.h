#pragma once

#include "Types.h"

#define DEFAULT_MODEL_LOCATION "M:\\LLM\\default_model.gguf"

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

	extern const string DialogueTag;
	extern const string ActionTag;
	extern const string ThoughtTag;
	extern const string NarrationTag;
	extern const string DirectionTag;

	extern const string DialogueTagEnd;
	extern const string ActionTagEnd;
	extern const string ThoughtTagEnd;
	extern const string NarrationTagEnd;
	extern const string DirectionTagEnd;

	extern const int ContextSize;
	extern const int MaxResponseLength;
	extern const float ContextWindowKeepRatio;
}
