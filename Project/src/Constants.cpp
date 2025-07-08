#include "Constants.h"
#include <SDL3/SDL.h>

// UI constants
const char* const Constants::AppTitle = "Llama chat";
int const Constants::WindowWidth = 1200;
int const Constants::WindowHeight = 900;
double const Constants::DefaultFontSize = 18.5;
double const Constants::StatusBarFontSize = 14.5;
double const Constants::ChatMessageFontSize = 15.5;

// Commands
string const Constants::DialogueTagBegin = "speak";
string const Constants::DialogueTagEnd = "/speak";
string const Constants::ActionTagBegin = "act";
string const Constants::ActionTagEnd = "/act";
string const Constants::NarrationTagBegin = "narration";
string const Constants::NarrationTagEnd = "/narration";
string const Constants::ThoughtTagBegin = "thought";
string const Constants::ThoughtTagEnd = "/thought";

// Generation constants
const int Constants::MaxMessageTokens = 512;
const float Constants::ContextWindowSizeRatio = 0.5f;