#include "gui/SolidBackgroundRenderer.h"

using namespace fig::gui;

SolidBackgroundRenderer::SolidBackgroundRenderer(Color color)
{
	SetColor(color);
}

void SolidBackgroundRenderer::Render(Renderer* pRenderer, Rectf rect)
{
	SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(pRenderer, &rect);
}

void SolidBackgroundRenderer::SetColor(Color color)
{
	_color = color;
}