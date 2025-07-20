#include "gui/Color.h"
#include "util/Utility.h"
#include <algorithm>

SDL_Color Color::White								{ 0xFF, 0xFF, 0xFF, 0xFF };
SDL_Color Color::Black								{ 0x00, 0x00, 0x00, 0xFF };
SDL_Color Color::DarkGray							{ 0x64, 0x64, 0x64, 0xFF };
SDL_Color Color::Transparent						{ 0xFF, 0xFF, 0xFF, 0x00 };
SDL_Color Color::Debug								{ 0xC0, 0x00, 0xC0, 0xFF };
SDL_Color Color::TextForeground						{ 0x00, 0x00, 0x00, 0xFF };
SDL_Color Color::TextSelectionForeground			{ 0xFF, 0xFF, 0xFF, 0xFF };
SDL_Color Color::TextSelectionBackground			{ 0x99, 0xC9, 0xEF, 0xFF };

SDL_Color Color::UserMessageBackground				{ 0xf2, 0xfb, 0xff, 255 };
SDL_Color Color::UserMessageBorder					{ 0x4d, 0xa1, 0xc1, 255 };
SDL_Color Color::BotMessageBackground				{ 0xff, 0xf3, 0xf9, 255 };
SDL_Color Color::BotMessageBorder					{ 0xef, 0x76, 0xbd, 255 };
SDL_Color Color::NarrationBackground				{ 0xf4, 0xf4, 0xf4, 255 };
SDL_Color Color::NarrationBorder					{ 0x9f, 0x9f, 0x9f, 255 };

//SDL_Color Color::AppBackground					{ 30, 30, 30, 255 };
//SDL_Color Color::ChatBackground					{ 40, 40, 40, 255 };
//SDL_Color Color::AppBackground					{ 255, 255, 255, 255 };
//SDL_Color Color::ChatBackground					{ 248, 248, 248, 255 };
SDL_Color Color::AppBackground						{ 0xfa, 0xf9, 0xf5, 255 };
SDL_Color Color::ChatBackground						{ 0xfa, 0xf9, 0xf5, 255 };

bool Color::IsDefined(SDL_Color color)
{
	return color.r != 0 || color.g != 0 || color.b != 0 || color.a != 0;
}

SDL_Color Color::WithAlpha(SDL_Color color, Uint8 alpha)
{
	return SDL_Color { color.r, color.g, color.b, alpha };
}

SDL_Color Color::AddRGB(SDL_Color colorA, SDL_Color colorB)
{
	float r = std::clamp(toF(colorA.r) + toF(colorB.r), 0.0f, 255.0f);
	float g = std::clamp(toF(colorA.g) + toF(colorB.g), 0.0f, 255.0f);
	float b = std::clamp(toF(colorA.b) + toF(colorB.b), 0.0f, 255.0f);

	return SDL_Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		colorA.a
	};
}

SDL_Color Color::AddRGB(SDL_Color color, int value)
{
	int r = std::clamp(toI(color.r) + value, 0, 255);
	int g = std::clamp(toI(color.g) + value, 0, 255);
	int b = std::clamp(toI(color.b) + value, 0, 255);

	return SDL_Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

SDL_Color Color::AddRGB(SDL_Color color, float value)
{
	float r = std::clamp(toF(color.r) + value * 255.0f, 0.0f, 255.0f);
	float g = std::clamp(toF(color.g) + value * 255.0f, 0.0f, 255.0f);
	float b = std::clamp(toF(color.b) + value * 255.0f, 0.0f, 255.0f);

	return SDL_Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}

SDL_Color Color::MultiplyRGB(SDL_Color colorA, SDL_Color colorB)
{
	return SDL_Color {
		static_cast<uint8_t>(toF(colorA.r) * toF(colorB.r) / 255.0f),
		static_cast<uint8_t>(toF(colorA.g) * toF(colorB.g) / 255.0f),
		static_cast<uint8_t>(toF(colorA.b) * toF(colorB.b) / 255.0f),
		colorA.a
	};
}

SDL_Color Color::MultiplyRGB(SDL_Color color, float value)
{
	float r = std::clamp(toF(color.r) * value, 0.0f, 255.0f);
	float g = std::clamp(toF(color.g) * value, 0.0f, 255.0f);
	float b = std::clamp(toF(color.b) * value, 0.0f, 255.0f);

	return SDL_Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}