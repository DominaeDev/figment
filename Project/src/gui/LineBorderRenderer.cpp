#include <pch.h>
#include "gui/LineBorderRenderer.h"

namespace fig::gui
{
	LineBorderRenderer::LineBorderRenderer(Color color, Directions directions) : CustomRenderer(color),
		_directions { directions }
	{
	}

	void LineBorderRenderer::Render(Renderer* pRenderer, const Rectf& rect)
	{
		SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, _color.a);
		
		if (_directions.IsSet(Direction::North))
			SDL_RenderLine(pRenderer, rect.x, rect.y, rect.x + rect.w - 1, rect.y);
		if (_directions.IsSet(Direction::East))
			SDL_RenderLine(pRenderer, rect.x + rect.w - 1, rect.y, rect.x + rect.w - 1, rect.y + rect.h - 1);
		if (_directions.IsSet(Direction::South))
			SDL_RenderLine(pRenderer, rect.x, rect.y + rect.h - 1, rect.x + rect.w - 1, rect.y + rect.h - 1);
		if (_directions.IsSet(Direction::West))
			SDL_RenderLine(pRenderer, rect.x, rect.y, rect.x, rect.y + rect.h - 1);
	}
}