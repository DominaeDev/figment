#include <pch.h>
#include "gui/VerticalListSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::gui;
using namespace fig::common_util;

void VerticalListSizer::OnLayout(Rectf parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int y = 0;
	for (auto it = _items.rbegin(); it != _items.rend(); ++it)
	{
		auto& item = *it;
		if (item.pControl == nullptr)
			continue;

		auto& frame = *item.pControl;
		auto& rect = frame.GetRect();
		int height = ceil_int(frame.GetHeight());

		rect.x = parentRect.x;

		if ((item.flags & Flag::Expand) != 0)
			rect.w = parentRect.w;

		rect.y = parentRect.y + parentRect.h - y - rect.h - _marginBottom;

		frame.SetPosition(rect.x - parentRect.x, rect.y - parentRect.y);
		frame.SetSize(rect.w, rect.h);

		y += height + _spacing;
	}
	_totalListHeight = toF(y - _spacing);
}