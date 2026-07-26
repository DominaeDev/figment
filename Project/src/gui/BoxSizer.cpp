#include <pch.h>
#include "gui/BoxSizer.h"
#include "gui/Control.h"

namespace fig::gui
{
	void BoxSizer::OnLayout(const fig::rect& parentRect)
	{
		this->parentRect = parentRect;

		auto count = GetCount();
		if (count == 0)
			return;

		int remainingSpace = GetAvailableSpace();
		int totalProportion = 0;
		int numStretch = 0;

		std::map<SizerItem::SizerTarget*, fig::coord> flexItems;

		auto items = GetLayoutItems();

		for (auto& item : items)
		{
			auto pControl = item.GetControl();

			if (item.info.prop == 0)
			{
				if (item.info.IsSet(SizerFlag::FixedSize))
					remainingSpace = std::max(remainingSpace - item.info.border, 0);
				else if (pControl != nullptr)
					remainingSpace = std::max(remainingSpace - GetItemSize(item, true), 0);
			}
			else if (item.info.prop > 0)
				totalProportion += item.info.prop;
			else if (item.info.IsSet(SizerFlag::Greedy) and pControl != nullptr)
			{
				auto size = std::min(GetItemSize(item, true), remainingSpace);
				flexItems[&item.target] = size;
				remainingSpace -= size;
			}
			else
				numStretch++;
		}
		if (totalProportion == 0)
			totalProportion = 1;

		position = 0;
		for (auto& item : items)
		{
			auto pControl = item.GetControl();
			auto& info = item.info;
			int size = 0;
			if (info.prop == 0)
			{
				if (info.IsSet(SizerFlag::FixedSize))
					size = info.border;
				else if (pControl)
					size = GetItemSize(item, true);
			}
			else if (info.prop > 0)
				size = ceil_int(info.prop * remainingSpace / (float)totalProportion);
			else if (item.info.IsSet(SizerFlag::Greedy))
				size = flexItems[&item.target];
			else
				size = ceil_int(remainingSpace / (float)numStretch);

			auto [min, max] = GetItemMinMaxSize(item); //! Move?
			if (min > 0)
				size = std::max(size, min);
			if (max > 0)
				size = std::min(size, max);

			fig::rect allocatedRect = AllocateRect(size);
			ApplyBorder(allocatedRect, item);
			item.rect = allocatedRect;

			if (pControl)
			{
				fig::rect rect
				{
					.x = allocatedRect.x,
					.y = allocatedRect.y,
					.w = pControl->GetWidth(),
					.h = pControl->GetHeight()
				};

				if (info.IsSet(SizerFlag::Fill))
				{
					rect.w = allocatedRect.w;
					rect.h = allocatedRect.h;
				}
				else if (info.IsSet(SizerFlag::Expand))
				{
					ExpandRect(rect, allocatedRect);
				}

				ClampRect(rect, item);
				AlignRect(rect, allocatedRect, item);
				
				OnLayoutItem(rect, item);
				pControl->SetRect(rect);
			}
		}
	}
}