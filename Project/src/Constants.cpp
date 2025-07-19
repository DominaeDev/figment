#include "Constants.h"
#include <SDL3/SDL.h>

// UI constants
const char* const Constants::AppTitle = "Llama chat";
int const Constants::WindowWidth = 1200;
int const Constants::WindowHeight = 900;
double const Constants::DefaultFontSize = 18.5;
double const Constants::StatusBarFontSize = 14.5;
double const Constants::ChatMessageFontSize = 16; //15.5;

// Commands
string const Constants::DialogueTag		= "talk";
string const Constants::ActionTag		= "act";
string const Constants::ThoughtTag		= "think";
string const Constants::NarrationTag	= "narrator";
string const Constants::DirectionTag	= "director";

string const Constants::DialogueTagEnd	= "/" + Constants::DialogueTag;
string const Constants::ActionTagEnd	= "/" + Constants::ActionTag;
string const Constants::NarrationTagEnd	= "/" + Constants::NarrationTag;
string const Constants::ThoughtTagEnd	= "/" + Constants::ThoughtTag;
string const Constants::DirectionTagEnd	= "/" + Constants::DirectionTag;

// Generation constants
const int Constants::MaxResponseLength = 368;
const float Constants::ContextWindowKeepRatio = 0.75f;