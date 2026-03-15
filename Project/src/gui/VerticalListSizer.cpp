#include <pch.h>
#include "gui/VerticalListSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::gui;
using namespace fig::util;

void VerticalListSizer::OnLayout(Rect parentRect)
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
		auto rect = control.GetRect();
		int height = control.GetHeight();

		rect.x = parentRect.x;

		if ((item.flags & Flag::Expand) != 0)
			rect.w = parentRect.w;

		rect.y = parentRect.y + parentRect.h - y - rect.h - _marginBottom;

		control.SetRect(rect);

		y += height + _spacing;
	}
	_totalListHeight = toF(y - _spacing);
}