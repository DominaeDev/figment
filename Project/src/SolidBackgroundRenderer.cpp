#include "SolidBackgroundRenderer.h"

SolidBackgroundRenderer::SolidBackgroundRenderer(SDL_Color color)
{
	SetColor(color);
}

void SolidBackgroundRenderer::DrawBackground(SDL_Renderer* pRenderer, SDL_FRect rect)
{
	SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(pRenderer, &rect);
}

void SolidBackgroundRenderer::SetColor(SDL_Color color)
{
	_color = color;
}