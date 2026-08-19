#include <pch.h>
#include "gui/LineBorderRenderer.h"

namespace fig::gui
{
	LineBorderRenderer::LineBorderRenderer(fig::color color, Directions directions) : CustomRenderer(color),
		_directions { directions }
	{
	}
	LineBorderRenderer::LineBorderRenderer(fig::color color, Direction direction) : CustomRenderer(color),
		_directions { direction }
	{
	}

	void LineBorderRenderer::Render(fig::renderer_ptr pRenderer, const fig::rectf& rect)
	{
		SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
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