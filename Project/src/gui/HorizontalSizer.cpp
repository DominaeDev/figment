#include <pch.h>
#include "gui/HorizontalSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::gui;
using namespace fig::util;

void HorizontalSizer::OnLayout(Rect parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	auto items = GetLayoutItems();
	int totalWidth = std::max(parentRect.w, 0);
	int itemWidth = ceil_int((float)totalWidth / count);
	int remainingWidth = totalWidth;
	int totalProportion = 0;
	int numStretch = 0;
	for (auto& item : items)
	{
		if (item.prop == 0 && item.pControl != nullptr)
			remainingWidth = std::max(remainingWidth - (item.pControl->GetWidth() + item.leftBorder() + item.rightBorder()), 0);
		else if (item.prop > 0)
			totalProportion += item.prop;
		else
			numStretch++;
	}
	if (totalProportion == 0)
		totalProportion = 1;

	Coord x = 0;
	for (auto& item : items)
	{
		auto& control = *item.pControl;
		auto rect = control.GetRect();
		Coord width = 0;
		if (item.prop == 0)
			width = control.GetWidth() + item.leftBorder() + item.rightBorder();
		else if (item.prop > 0)
			width = ceil_int(item.prop * remainingWidth / (float)totalProportion);
		else
			width = ceil_int(remainingWidth / (float)numStretch);

		if (control.GetMinSize().x > 0)
			width = std::max(width, control.GetMinSize().x);
		if (control.GetMaxSize().x > 0)
			width = std::min(width, control.GetMaxSize().x);

		Rect borderRect {
			parentRect.x + x,
			parentRect.y,
			width,
			parentRect.h,
		};

		if ((item.flags & Flag::Left) != 0)
		{
			borderRect.x += item.border;
			borderRect.w -= item.border;
		}
		if ((item.flags & Flag::Top) != 0)
		{
			borderRect.y += item.border;
			borderRect.h -= item.border;
		}
		if ((item.flags & Flag::Right) != 0)
		{
			borderRect.w -= item.border;
		}
		if ((item.flags & Flag::Bottom) != 0)
		{
			borderRect.h -= item.border;
		}

		rect.x = borderRect.x;
		rect.y = borderRect.y;
		if (item.prop != 0)
			rect.w = borderRect.w;

		if ((item.flags & Flag::Expand) != 0)
		{
			rect.h = borderRect.h;
		}
		else
		{
			if ((item.flags & Flag::AlignLeft) != 0)
				rect.x = borderRect.x;
			else if ((item.flags & Flag::AlignCenterHorizontal) != 0)
				rect.x = borderRect.x + (borderRect.w - rect.w) / 2;
			else if ((item.flags & Flag::AlignRight) != 0)
				rect.x = borderRect.x + borderRect.w - rect.w;
			if ((item.flags & Flag::AlignTop) != 0)
				rect.y = borderRect.y;
			else if ((item.flags & Flag::AlignCenterVertical) != 0)
				rect.y = borderRect.y + (borderRect.h - rect.h) / 2;
			else if ((item.flags & Flag::AlignBottom) != 0)
				rect.y = borderRect.y + borderRect.h - rect.h;
		}

		control.SetRect(rect);

		x += width;
	}
}