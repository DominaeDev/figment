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
string const Constants::DialogueTagBegin = "talk";
string const Constants::DialogueTagEnd = "/talk";
string const Constants::ActionTagBegin = "act";
string const Constants::ActionTagEnd = "/act";
string const Constants::NarrationTagBegin = "narrator";
string const Constants::NarrationTagEnd = "/narrator";
string const Constants::ThoughtTagBegin = "think";
string const Constants::ThoughtTagEnd = "/think";

// Generation constants
const int Constants::MaxMessageTokens = 512;
const float Constants::ContextWindowSizeRatio = 0.5f;