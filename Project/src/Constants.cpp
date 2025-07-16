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
string const Constants::DialogueTag		= "talk";
string const Constants::ActionTag		= "act";
string const Constants::NarrationTag	= "narration";
string const Constants::ThoughtTag		= "think";

string const Constants::DialogueTagEnd	= "/" + Constants::DialogueTag;
string const Constants::ActionTagEnd	= "/" + Constants::ActionTag;
string const Constants::NarrationTagEnd	= "/" + Constants::NarrationTag;
string const Constants::ThoughtTagEnd	= "/" + Constants::ThoughtTag;

// Generation constants
const int Constants::MaxMessageTokens = 512;
const float Constants::ContextWindowSizeRatio = 0.5f;