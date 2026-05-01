#include <pch.h>
#include "gui/SolidFillRenderer.h"

namespace fig::gui
{
	SolidFillRenderer::SolidFillRenderer(Color color) : CustomRenderer(color)
	{
	}

	void SolidFillRenderer::Render(Renderer* pRenderer, const Rectf& rect)
	{
		if (_color.a == 0xFF)
		{
			SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(pRenderer, &rect);
		}
		else
		{
			SDL_BlendMode mode;
			SDL_GetRenderDrawBlendMode(pRenderer, &mode);
			bool b = SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, _color.a);
			SDL_RenderFillRect(pRenderer, &rect);
			SDL_SetRenderDrawBlendMode(pRenderer, mode);
		}
	}
}