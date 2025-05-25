#include "Color.h"

SDL_Color Color::White								{ 0xFF, 0xFF, 0xFF, 0xFF };
SDL_Color Color::Black								{ 0x00, 0x00, 0x00, 0xFF };
SDL_Color Color::Transparent						{ 0xFF, 0xFF, 0xFF, 0x00 };
SDL_Color Color::Debug								{ 0xC0, 0x00, 0xC0, 0xFF };

SDL_Color Color::TextForeground						{ 0x00, 0x00, 0x00, 0xFF };
SDL_Color Color::TextSelectionForeground			{ 0xFF, 0xFF, 0xFF, 0xFF };
SDL_Color Color::TextSelectionBackground			{ 0x99, 0xC9, 0xEF, 0xFF };

bool Color::IsDefined(SDL_Color color)
{
	return color.r != 0 || color.g != 0 || color.b != 0 || color.a != 0;
}

SDL_Color Color::WithAlpha(SDL_Color color, Uint8 alpha)
{
	return SDL_Color { color.r, color.g, color.b, alpha };
}