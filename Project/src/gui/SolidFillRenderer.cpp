#include <pch.h>
#include "gui/SolidFillRenderer.h"

namespace fig::gui
{
	SolidFillRenderer::SolidFillRenderer(Color color)
	{
		SetColor(color);
	}

	void SolidFillRenderer::Render(Renderer* pRenderer, Rectf rect)
	{
		SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(pRenderer, &rect);
	}

	void SolidFillRenderer::SetColor(Color color)
	{
		_color = color;
	}
}