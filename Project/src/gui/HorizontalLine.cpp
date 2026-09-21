#include <pch.h>
#include "gui/HorizontalLine.h"

namespace fig::gui
{
	HorizontalLine::HorizontalLine(ControlPtr pParent, fig::color_ref_with_alpha color) : Control(pParent)
	{
		SetForegroundColor(color);
		SetHeight(16);
	}

	void HorizontalLine::OnRender(fig::renderer_ptr pRenderer)
	{
		auto fgColor = GetForegroundColor();
		SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(pRenderer, fgColor.r(), fgColor.g(), fgColor.b(), fgColor.a());

		auto& rect = GetRect();
		SDL_RenderLine(pRenderer, toF(rect.x), toF(rect.y + (rect.h / 2)), toF(rect.x + rect.w), toF(rect.y + (rect.h / 2)));
	}
}