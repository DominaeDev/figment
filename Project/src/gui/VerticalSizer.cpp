#include <pch.h>
#include "gui/VerticalSizer.h"
#include "gui/Control.h"
#include <limits>

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
		else if (auto pSizer = item.GetSizer())
			return item.rect.h + (includeBorder ? item.GetTopBorder() + item.GetBottomBorder() : 0);
		return 0;
	}

	std::pair<fig::coord, fig::coord> VerticalSizer::GetItemMinMaxSize(SizerItem& item)
	{
		if (auto pControl = item.GetControl())
			return std::make_pair(pControl->GetMinHeight(), pControl->GetMaxHeight());
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

	void VerticalSizer::OnLayoutItem(fig::rect& itemRect, SizerItem& item)
	{
		if (item.info.IsSet(SizerFlag::Expand))
		{
			if (auto pText = item.GetControl<StaticText>())
			{
				if (pText->IsWordWrapEnabled())
				{
					auto [prev_w, prev_h] = pText->MeasureText(true);
					pText->SetMaxLineWidth(itemRect.w);
					auto [new_w, new_h] = pText->MeasureText(true);
					position += new_h - prev_h;
					item.rect.h = new_h;

					pText->InvalidateText();
				}
				else if (pText->IsEllipsisEnabled())
				{
					pText->InvalidateText();
				}
			}
		}
	}
}

