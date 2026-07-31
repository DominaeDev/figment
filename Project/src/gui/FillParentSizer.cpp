#include <pch.h>
#include "gui/FillParentSizer.h"

namespace fig::gui
{
	void FillParentSizer::OnLayout(const fig::rect& parentRect)
	{
		auto items = GetLayoutItems();

		for (auto& item : items)
		{
			auto pControl = item.GetControl();
			auto& info = item.info;

			fig::rect allocatedRect = parentRect;
			ApplyBorder(allocatedRect, item);
			item.rect = allocatedRect;

			if (pControl)
			{
				fig::rect rect = allocatedRect;

				ClampRect(rect, item);
				AlignRect(rect, allocatedRect, item);

				OnLayoutItem(rect, item);
				pControl->SetRect(rect);
			}
		}
	}
}