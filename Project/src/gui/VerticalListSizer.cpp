#include <pch.h>
#include "gui/VerticalListSizer.h"
#include "gui/Control.h"

namespace fig::gui
{
	void VerticalListSizer::OnLayout(const fig::rect& parentRect)
	{
		auto count = GetCount();
		if (count == 0)
			return;

		auto items = GetLayoutItems()
			| std::views::reverse;

		int y = 0;
		for (auto& item : items)
		{
			auto pControl = item.GetControl();

			if (pControl)
			{
				auto rect = pControl->GetRect();
				int height = pControl->GetHeight();

				rect.x = parentRect.x;
				if ((item.info.flags & Flag::Expand) != 0)
					rect.w = parentRect.w;
				rect.y = parentRect.y + parentRect.h - y - rect.h - _marginBottom;

				pControl->SetRect(rect);
				item.rect = rect;

				y += height + _spacing;
			}
		}
		_totalListHeight = toF(y - _spacing);
	}
}