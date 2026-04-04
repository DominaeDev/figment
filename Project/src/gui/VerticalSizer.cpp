#include <pch.h>
#include "gui/VerticalSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::gui;
using namespace fig::util;

void VerticalSizer::OnLayout(const Rect& parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int remainingHeight = std::max(parentRect.h, 0);
	int itemHeight = ceil_int((float)remainingHeight / count);
	int totalProportion = 0;
	int numStretch = 0;

	auto items = GetLayoutItems();

	for (auto& item : items)
	{
		auto pControl = item.GetControl();

		if (item.info.prop == 0)
		{
			if ((item.info.flags & Sizer::FixedSize) != 0)
				remainingHeight = std::max(remainingHeight - item.info.border, 0);
			else if (pControl != nullptr)
				remainingHeight = std::max(remainingHeight - (pControl->GetHeight() + item.info.topBorder() + item.info.bottomBorder()), 0);
		}
		else if (item.info.prop > 0)
			totalProportion += item.info.prop;
		else
			numStretch++;
	}
	if (totalProportion == 0)
		totalProportion = 1;

	int y = 0;
	for (auto& item : items)
	{
		auto pControl = item.GetControl();
		auto& info = item.info;
		int height = 0;
		if (info.prop == 0)
		{
			if ((info.flags & Sizer::FixedSize) != 0)
				height = info.border;
			else if (pControl)
				height = pControl->GetHeight() + info.topBorder() + info.bottomBorder();
		}
		else if (info.prop > 0)
			height = ceil_int(info.prop * remainingHeight / (float)totalProportion);
		else
			height = ceil_int(remainingHeight / (float)numStretch);

		if (pControl and pControl->GetMinSize().y > 0) //! Move?
			height = std::max(height, pControl->GetMinSize().y);
		if (pControl and pControl->GetMaxSize().y > 0) //! Move?
			height = std::min(height, pControl->GetMaxSize().y);

		Rect innerRect {
			parentRect.x,
			parentRect.y + y,
			parentRect.w,
			height,
		};

		if ((info.flags & Flag::Left) != 0)
		{
			innerRect.x += info.border;
			innerRect.w -= info.border;
		}
		if ((info.flags & Flag::Top) != 0)
		{
			innerRect.y += info.border;
			innerRect.h -= info.border;
		}
		if ((info.flags & Flag::Right) != 0)
		{
			innerRect.w -= info.border;
		}
		if ((info.flags & Flag::Bottom) != 0)
		{
			innerRect.h -= info.border;
		}
		item.rect = innerRect;

		if (pControl)
		{
			auto rect = pControl->GetRect();

			rect.x = innerRect.x;
			rect.y = innerRect.y;

			if ((info.flags & Flag::Fill) != 0)
			{
				rect.w = innerRect.w;
				rect.h = innerRect.h;
			}
			else if ((info.flags & Flag::Expand) != 0)
			{
				rect.w = innerRect.w;
			}

			if ((info.flags & Flag::AlignTop) != 0)
				rect.y = innerRect.y;
			else if ((info.flags & Flag::AlignCenterVertical) != 0)
				rect.y = innerRect.y + (innerRect.h - rect.h) / 2;
			else if ((info.flags & Flag::AlignBottom) != 0)
				rect.y = innerRect.y + innerRect.h - rect.h;
			if ((info.flags & Flag::AlignLeft) != 0)
				rect.x = innerRect.x;
			else if ((info.flags & Flag::AlignCenterHorizontal) != 0)
				rect.x = innerRect.x + (innerRect.w - rect.w) / 2;
			else if ((info.flags & Flag::AlignRight) != 0)
				rect.x = innerRect.x + innerRect.w - rect.w;
			pControl->SetRect(rect);
		}

		y += height;
	}
}