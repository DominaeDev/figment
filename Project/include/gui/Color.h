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

	extern Color MessageBorderDefault;
	extern Color MessageBackgroundDefault;
	extern Color MessageBorderBlue;
	extern Color MessageBackgroundBlue;
	extern Color MessageBorderPink;
	extern Color MessageBackgroundPink;
	extern Color MessageBorderGreen;
	extern Color MessageBackgroundGreen;
	extern Color MessageBorderYellow;
	extern Color MessageBackgroundYellow;
	extern Color MessageBorderRed;
	extern Color MessageBackgroundRed;
	extern Color MessageBorderTeal;
	extern Color MessageBackgroundTeal;
	extern Color MessageBorderPurple;
	extern Color MessageBackgroundPurple;
	extern Color MessageBorderBrown;
	extern Color MessageBackgroundBrown;
	extern Color MessageBorderNavy;
	extern Color MessageBackgroundNavy;

	extern Color DefaultUserMessageBorder;
	extern Color DefaultUserMessageBackground;
	extern std::array<Color, 8> DefaultBotMessageBorders;
	extern std::array<Color, 8> DefaultBotMessageBackgrounds;
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
	static Color with_alpha(Color color, Uint8 alpha);

	static Color color_from_string(std::string hex);
	static void color_to_hsv(Color color, float& h, float& s, float& v);
	static Color hsv_to_color(float h, float s, float v);

	color_util() = delete;
};