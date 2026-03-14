#include <pch.h>
#include "gui/VerticalSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::gui;
using namespace fig::util;

void VerticalSizer::OnLayout(Rectf parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int totalHeight = ceil_int(std::max(parentRect.h, 0.0f));
	int itemHeight= ceil_int((float)totalHeight/ count);
	int remainingHeight = totalHeight;
	int totalProportion = 0;
	int numStretch = 0;

	auto items = GetLayoutItems();

	for (auto& item : items)
	{
		if (item.prop == 0 && item.pControl != nullptr)
		{
			remainingHeight = ceil_int(std::max(remainingHeight - (item.pControl->GetHeight() + item.topBorder() + item.bottomBorder()), 0.0f));
		}
		else if (item.prop > 0)
			totalProportion += item.prop;
		else
			numStretch++;
	}
	if (totalProportion == 0)
		totalProportion = 1;

	int y = 0;
	for (auto& item : items)
	{
		auto& control = *item.pControl;
		auto& rect = control.GetRect();
		int height = 0;
		if (item.prop == 0)
			height = ceil_int(control.GetHeight() + item.topBorder() + item.bottomBorder());
		else if (item.prop > 0)
			height = ceil_int(item.prop * remainingHeight / (float)totalProportion);
		else
			height = ceil_int(remainingHeight / (float)numStretch);

		if (control.GetMinSize().y > 0)
			height = ceil_int(std::max((float)height, control.GetMinSize().y));
		if (control.GetMaxSize().y > 0)
			height = ceil_int(std::min((float)height, control.GetMaxSize().y));

		Rectf borderRect {
			parentRect.x,
			parentRect.y + y,
			parentRect.w,
			(float)height,
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
		rect.h = borderRect.h;

		if ((item.flags & Flag::Expand) != 0)
		{
			rect.w = borderRect.w;
		}
		else
		{
			if ((item.flags & Flag::AlignTop) != 0)
				rect.y = borderRect.y;
			else if ((item.flags & Flag::AlignCenterVertical) != 0)
				rect.y = borderRect.y + (borderRect.h - rect.h) / 2;
			else if ((item.flags & Flag::AlignBottom) != 0)
				rect.y = borderRect.y + borderRect.h - rect.h;
			if ((item.flags & Flag::AlignLeft) != 0)
				rect.x = borderRect.x;
			else if ((item.flags & Flag::AlignCenterHorizontal) != 0)
				rect.x = borderRect.x + (borderRect.w - rect.w) / 2;
			else if ((item.flags & Flag::AlignRight) != 0)
				rect.x = borderRect.x + borderRect.w - rect.w;
		}

		y += height;

		control.SetPosition(rect.x - parentRect.x, rect.y - parentRect.y);
		control.SetSize(rect.w, rect.h);
	}	
}