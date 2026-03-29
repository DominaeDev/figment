#include <pch.h>
#include "gui/HorizontalSizer.h"
#include "gui/LayoutElement.h"
#include "util/Common.h"

using namespace fig::util;

namespace fig::gui
{
	void HorizontalSizer::OnLayout(const Rect& parentRect)
	{
		auto count = GetCount();
		if (count == 0)
			return;

		auto items = GetLayoutItems();
		int remainingWidth = std::max(parentRect.w, 0);
		int itemWidth = ceil_int((float)remainingWidth / count);
		int totalProportion = 0;
		int numStretch = 0;

		for (auto& item : items)
		{
			auto pControl = item.GetControl();
			if (item.info.prop == 0)
			{
				if (pControl != nullptr)
					remainingWidth = std::max(remainingWidth - (pControl->GetWidth() + item.info.leftBorder() + item.info.rightBorder()), 0);
				else
					remainingWidth = std::max(remainingWidth - (item.info.fixed + item.info.leftBorder() + item.info.rightBorder()), 0);
			}
			else if (item.info.prop > 0)
				totalProportion += item.info.prop;
			else
				numStretch++;
		}
		if (totalProportion == 0)
			totalProportion = 1;

		Coord x = 0;
		for (auto& item : items)
		{
			auto pControl = item.GetControl();
			auto& info = item.info;
			Coord width = 0;
			if (info.prop == 0)
			{
				if (pControl)
					width = pControl->GetWidth() + info.leftBorder() + info.rightBorder();
				else
					width = info.fixed + info.leftBorder() + info.rightBorder();
			}
			else if (info.prop > 0)
				width = ceil_int(info.prop * remainingWidth / (float)totalProportion);
			else
				width = ceil_int(remainingWidth / (float)numStretch);

			if (pControl and pControl->GetMinSize().x > 0) //! Move?
				width = std::max(width, pControl->GetMinSize().x);
			if (pControl and pControl->GetMaxSize().x > 0) //! Move?
				width = std::min(width, pControl->GetMaxSize().x);

			Rect innerRect {
				parentRect.x + x,
				parentRect.y,
				width,
				parentRect.h,
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
					rect.h = innerRect.h;
				}

				if ((info.flags & Flag::AlignLeft) != 0)
					rect.x = innerRect.x;
				else if ((info.flags & Flag::AlignCenterHorizontal) != 0)
					rect.x = innerRect.x + (innerRect.w - rect.w) / 2;
				else if ((info.flags & Flag::AlignRight) != 0)
					rect.x = innerRect.x + innerRect.w - rect.w;

				if ((info.flags & Flag::AlignTop) != 0)
					rect.y = innerRect.y;
				else if ((info.flags & Flag::AlignCenterVertical) != 0)
					rect.y = innerRect.y + (innerRect.h - rect.h) / 2;
				else if ((info.flags & Flag::AlignBottom) != 0)
					rect.y = innerRect.y + innerRect.h - rect.h;

				pControl->SetRect(rect);
			}

			x += width;
		}
	}
}