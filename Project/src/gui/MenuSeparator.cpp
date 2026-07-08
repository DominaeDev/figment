#include <pch.h>
#include "gui/MenuSeparator.h"

namespace fig::gui
{
	MenuSeparator::MenuSeparator(ParentPtr pParent) : Control(pParent)
	{
	}

	void MenuSeparator::OnRender(Renderer* pRenderer)
	{
		auto fgColor = GetForegroundColor();
		auto rect = GetRect();

		SDL_SetRenderDrawColor(pRenderer, fgColor.r, fgColor.g, fgColor.b, fgColor.a);
		SDL_RenderLine(pRenderer, toF(rect.x), toF(rect.y + rect.h / 2), toF(rect.x + rect.w - 1), toF(rect.y + rect.h / 2));
	}
}