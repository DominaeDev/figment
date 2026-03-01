#include <pch.h>
#include "gui/VerticalListSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::gui;
using namespace fig::util;

void VerticalListSizer::OnLayout(Rectf parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	auto items = GetLayoutItems()
		| std::views::reverse;

	int y = 0;
	for (auto it = items.begin(); it != items.end(); ++it)
	{
		auto& item = *it;

		auto& control = *item.pControl;
		auto& rect = control.GetRect();
		int height = ceil_int(control.GetHeight());

		rect.x = parentRect.x;

		if ((item.flags & Flag::Expand) != 0)
			rect.w = parentRect.w;

		rect.y = parentRect.y + parentRect.h - y - rect.h - _marginBottom;

		control.SetPosition(rect.x - parentRect.x, rect.y - parentRect.y);
		control.SetSize(rect.w, rect.h);

		y += height + _spacing;
	}
	_totalListHeight = toF(y - _spacing);
}