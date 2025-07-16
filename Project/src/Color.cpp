#include "Color.h"
#include "Utility.h"

SDL_Color Color::White								{ 0xFF, 0xFF, 0xFF, 0xFF };
SDL_Color Color::Black								{ 0x00, 0x00, 0x00, 0xFF };
SDL_Color Color::DarkGray							{ 0x64, 0x64, 0x64, 0xFF };
SDL_Color Color::Transparent						{ 0xFF, 0xFF, 0xFF, 0x00 };
SDL_Color Color::Debug								{ 0xC0, 0x00, 0xC0, 0xFF };
SDL_Color Color::TextForeground						{ 0x00, 0x00, 0x00, 0xFF };
SDL_Color Color::TextSelectionForeground			{ 0xFF, 0xFF, 0xFF, 0xFF };
SDL_Color Color::TextSelectionBackground			{ 0x99, 0xC9, 0xEF, 0xFF };

SDL_Color Color::UserMessageBackground				{ 185, 219, 232, 255 };
SDL_Color Color::BotMessageBackground				{ 255, 219, 238, 255 };
SDL_Color Color::NarrationBackground				{ 180, 180, 180, 255 };

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
	float r = clamp(toF(colorA.r) + toF(colorB.r), 0.0f, 255.0f);
	float g = clamp(toF(colorA.g) + toF(colorB.g), 0.0f, 255.0f);
	float b = clamp(toF(colorA.b) + toF(colorB.b), 0.0f, 255.0f);

	return SDL_Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		colorA.a
	};
}

SDL_Color Color::AddRGB(SDL_Color color, float value)
{
	float r = clamp(toF(color.r) + value * 255.0f, 0.0f, 255.0f);
	float g = clamp(toF(color.g) + value * 255.0f, 0.0f, 255.0f);
	float b = clamp(toF(color.b) + value * 255.0f, 0.0f, 255.0f);

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
	float r = clamp(toF(color.r) * value, 0.0f, 255.0f);
	float g = clamp(toF(color.g) * value, 0.0f, 255.0f);
	float b = clamp(toF(color.b) * value, 0.0f, 255.0f);

	return SDL_Color {
		static_cast<uint8_t>(r),
		static_cast<uint8_t>(g),
		static_cast<uint8_t>(b),
		color.a
	};
}