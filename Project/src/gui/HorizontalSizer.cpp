#include <pch.h>
#include "gui/HorizontalSizer.h"
#include "gui/Control.h"

namespace fig::gui
{
	fig::coord HorizontalSizer::GetAvailableSpace()
	{
		return std::max(parentRect.w, 0);
	}

	fig::coord HorizontalSizer::GetItemSize(SizerItem& item, bool includeBorder)
	{
		if (auto pControl = item.GetControl())
			return pControl->GetWidth() + (includeBorder ? item.GetLeftBorder() + item.GetRightBorder() : 0);
		return 0;
	}

	std::pair<fig::coord, fig::coord> HorizontalSizer::GetItemMinMaxSize(SizerItem& item)
	{
		if (auto pControl = item.GetControl())
			return std::make_pair(pControl->GetMinWidth(), pControl->GetMaxWidth());
		return {};
	}

	fig::rect HorizontalSizer::AllocateRect(fig::coord size)
	{
		fig::rect rect {
			parentRect.x + position,
			parentRect.y,
			size,
			parentRect.h,
		};

		position += size;
		return rect;
	}

	void HorizontalSizer::ExpandRect(fig::rect& rect, const fig::rect& allocated)
	{
		rect.h = allocated.h;
	}

	fig::coord HorizontalSizer::GetExtent() const
	{
		fig::coord min = std::numeric_limits<fig::coord>::max();
		fig::coord max = std::numeric_limits<fig::coord>::min();
		for (auto& item : GetLayoutItems())
		{
			min = std::min(min, item.rect.x);
			max = std::min(max, item.rect.x + item.rect.w);
		}
		return max - min;
	}
}

