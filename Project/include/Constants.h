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
	extern const double ChatMessageFontSize;

	extern const string DialogueTagBegin;
	extern const string DialogueTagEnd;
	extern const string ActionTagBegin;
	extern const string ActionTagEnd;
	extern const string NarrationTagBegin;
	extern const string NarrationTagEnd;
	extern const string ThoughtTagBegin;
	extern const string ThoughtTagEnd;

	extern const int MaxMessageTokens;
	extern const float ContextWindowSizeRatio;
}
