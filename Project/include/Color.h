#pragma once

#include "Types.h"

namespace Color
{
	extern SDL_Color Transparent;
	extern SDL_Color White;
	extern SDL_Color Black;
	extern SDL_Color Debug;

	extern SDL_Color TextForeground;
	extern SDL_Color TextSelectionForeground;
	extern SDL_Color TextSelectionBackground;

	extern bool IsDefined(SDL_Color color);
	extern SDL_Color WithAlpha(SDL_Color color, Uint8 alpha = 255u);

}