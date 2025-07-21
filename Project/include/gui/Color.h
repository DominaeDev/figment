#pragma once

#include "Graphics.h"

namespace Color
{
	extern SDL_Color Transparent;
	extern SDL_Color White;
	extern SDL_Color Black;
	extern SDL_Color DarkGray;
	extern SDL_Color Debug;

	extern SDL_Color AppBackground;
	extern SDL_Color ChatBackground;

	extern SDL_Color TextForeground;
	extern SDL_Color TextSelectionForeground;
	extern SDL_Color TextSelectionBackground;
	
	extern SDL_Color UserMessageBackground;
	extern SDL_Color UserMessageBorder;
	extern SDL_Color BotMessageBackground;
	extern SDL_Color BotMessageBorder;
	extern SDL_Color NarrationBackground;
	extern SDL_Color NarrationBorder;

	extern bool IsDefined(SDL_Color color);
	extern SDL_Color WithAlpha(SDL_Color color, Uint8 alpha = 255u);

	extern SDL_Color AddRGB(SDL_Color colorA, SDL_Color colorB);
	extern SDL_Color AddRGB(SDL_Color colorA, int value);
	extern SDL_Color AddRGB(SDL_Color colorA, float value);
	extern SDL_Color MultiplyRGB(SDL_Color colorA, SDL_Color colorB);
	extern SDL_Color MultiplyRGB(SDL_Color colorA, float value);

	extern SDL_FColor ColorToFColor(SDL_Color color);
	extern SDL_Color FColorToColor(SDL_FColor color);
}