#include "gui/Color.h"
#include "Types.h"
#include <algorithm>

Color Colors::White								{ 0xFF, 0xFF, 0xFF, 0xFF };
Color Colors::Black								{ 0x00, 0x00, 0x00, 0xFF };
Color Colors::DarkGray							{ 0x64, 0x64, 0x64, 0xFF };
Color Colors::Transparent						{ 0xFF, 0xFF, 0xFF, 0x00 };
Color Colors::Debug								{ 0xC0, 0x00, 0xC0, 0xFF };
Color Colors::TextForeground						{ 0x00, 0x00, 0x00, 0xFF };
Color Colors::TextSelectionForeground			{ 0xFF, 0xFF, 0xFF, 0xFF };
Color Colors::TextSelectionBackground			{ 0x99, 0xC9, 0xEF, 0xFF };

Color Colors::UserMessageBackground				{ 0xf2, 0xfb, 0xff, 255 };
Color Colors::UserMessageBorder					{ 0x4d, 0xa1, 0xc1, 255 };
Color Colors::BotMessageBackground				{ 0xff, 0xf3, 0xf9, 255 };
Color Colors::BotMessageBorder					{ 0xef, 0x76, 0xbd, 255 };
Color Colors::NarrationBackground				{ 0xf4, 0xf4, 0xf4, 255 };
Color Colors::NarrationBorder					{ 0x9f, 0x9f, 0x9f, 255 };

Color Colors::AppBackground						{ 0xfa, 0xf9, 0xf5, 255 };
Color Colors::ChatBackground					{ 0xfa, 0xf9, 0xf5, 255 };

bool color_util::is_defined(Color color)
{
	return color.r != 0 || color.g != 0 || color.b != 0 || color.a != 0;
}

Color color_util::with_alpha(Color color, Uint8 alpha)
{
	return Color { color.r, color.g, color.b, alpha };
}

Color color_util::add_rgb(Color colorA, Color colorB)
{
	float r = std::clamp(toF(colorA.r) + toF(colorB.r), 0.0f, 255.0f);
	float g = std::clamp(toF(colorA.g) + toF(colorB.g), 0.0f, 255.0f);
	float b = std::clamp(toF(colorA.b) + toF(colorB.b), 0.0f, 255.0f);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		colorA.a
	};
}

Color color_util::add_rgb(Color color, int value)
{
	int r = std::clamp(toI(color.r) + value, 0, 255);
	int g = std::clamp(toI(color.g) + value, 0, 255);
	int b = std::clamp(toI(color.b) + value, 0, 255);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

Color color_util::add_rgb(Color color, float value)
{
	float r = std::clamp(toF(color.r) + value * 255.0f, 0.0f, 255.0f);
	float g = std::clamp(toF(color.g) + value * 255.0f, 0.0f, 255.0f);
	float b = std::clamp(toF(color.b) + value * 255.0f, 0.0f, 255.0f);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

Color color_util::multiply_rgb(Color colorA, Color colorB)
{
	return Color {
		static_cast<uint8_t>(toF(colorA.r) * toF(colorB.r) / 255.0f),
		static_cast<uint8_t>(toF(colorA.g) * toF(colorB.g) / 255.0f),
		static_cast<uint8_t>(toF(colorA.b) * toF(colorB.b) / 255.0f),
		colorA.a
	};
}

Color color_util::multiply_rgb(Color color, float value)
{
	float r = std::clamp(toF(color.r) * value, 0.0f, 255.0f);
	float g = std::clamp(toF(color.g) * value, 0.0f, 255.0f);
	float b = std::clamp(toF(color.b) * value, 0.0f, 255.0f);

	return Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

Colorf color_util::to_colorf(Color color)
{
	return Colorf {
		color.r / 255.0f,
		color.g / 255.0f,
		color.b / 255.0f,
		color.a / 255.0f,
	};
}

Color color_util::to_color(Colorf color)
{
	return Color {
		std::clamp(static_cast<uint8_t>(color.r * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.g * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.b * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.a * 255.0f), 0_u8, 255_u8),
	};
}