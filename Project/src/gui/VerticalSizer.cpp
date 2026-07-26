#include <pch.h>
#include "gui/VerticalSizer.h"
#include "gui/Control.h"

namespace fig::gui
{
	fig::coord VerticalSizer::GetAvailableSpace()
	{
		return std::max(parentRect.h, 0);
	}

	fig::coord VerticalSizer::GetItemSize(SizerItem& item, bool includeBorder)
	{
		if (auto pControl = item.GetControl())
			return pControl->GetHeight() + (includeBorder ? item.GetTopBorder() + item.GetBottomBorder() : 0);
		return 0;
	}

	std::pair<fig::coord, fig::coord> VerticalSizer::GetItemMinMaxSize(SizerItem& item)
	{
		if (auto pControl = item.GetControl())
			return std::make_pair(pControl->GetMinSize().y, pControl->GetMaxSize().y);
		return {};
	}

	fig::rect VerticalSizer::AllocateRect(fig::coord size)
	{
		fig::rect rect {
			parentRect.x,
			parentRect.y + position,
			parentRect.w,
			size,
		};

		position += size;
		return rect;
	}

	void VerticalSizer::ExpandRect(fig::rect& rect, const fig::rect& allocated)
	{
		rect.w = allocated.w;
	}
}

