#pragma once

#include "Graphics.h"

namespace Colors
{
	extern Color Transparent;
	extern Color White;
	extern Color Black;
	extern Color DarkGray;
	extern Color Debug;

	extern Color AppBackground;
	extern Color ChatBackground;

	extern Color TextForeground;
	extern Color TextSelectionForeground;
	extern Color TextSelectionBackground;

	extern Color UserMessageBackground;
	extern Color UserMessageBorder;
	extern Color BotMessageBackground;
	extern Color BotMessageBorder;
	extern Color NarrationBackground;
	extern Color NarrationBorder;
}

struct color_util
{
	static bool is_defined(Color color);
	static Colorf to_colorf(Color color);
	static Color to_color(Colorf color);

	static Color add_rgb(Color colorA, Color colorB);
	static Color add_rgb(Color colorA, int value);
	static Color add_rgb(Color colorA, float value);
	static Color multiply_rgb(Color colorA, Color colorB);
	static Color multiply_rgb(Color colorA, float value);
	static Color with_alpha(Color color, Uint8 alpha = 255u);

	color_util() = delete;
};