#include <pch.h>
#include "gui/HorizontalLine.h"

namespace fig::gui
{
	HorizontalLine::HorizontalLine(ControlPtr pParent, fig::color color) : Control(pParent),
		_color { color }
	{
		SetHeight(16);
	}

	void HorizontalLine::OnRender(fig::renderer_ptr pRenderer)
	{
		SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(pRenderer, _color.r, _color.g, _color.b, _color.a);

		auto& rect = GetRect();
		SDL_RenderLine(pRenderer, toF(rect.x), toF(rect.y + (rect.h / 2)), toF(rect.x + rect.w), toF(rect.y + (rect.h / 2)));
	}
}