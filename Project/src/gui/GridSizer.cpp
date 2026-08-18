#include <pch.h>
#include "gui/GridSizer.h"
#include "gui/Control.h"

namespace fig::gui
{
	GridSizer::GridSizer(int32_t cellWidth, int32_t cellHeight, int32_t spacingX, int32_t spacingY) :
		_cellWidth { cellWidth },
		_cellHeight { cellHeight },
		_spacingX { spacingX },
		_spacingY { spacingY }
	{
	}

	void GridSizer::SetCellSize(int32_t x, int32_t y)
	{
		_cellWidth = std::max(x, 0);
		_cellHeight = std::max(y, 0);
	}

	void GridSizer::SetSpacing(int32_t x, int32_t y)
	{
		_spacingX = std::max(x, 0);
		_spacingY = std::max(y, 0);
	}

	void GridSizer::OnLayout(const fig::rect& parentRect)
	{
		auto count = GetCount();
		if (count == 0)
		{
			_last_columns = 0;
			_last_rows = 0;
			return;
		}

		int totalWidth = std::max(parentRect.w, 0);
		int columns;
		if (totalWidth > 0 && _cellWidth > 0)
			columns = std::max((totalWidth + _spacingX) / toI(_cellWidth + _spacingX), 1);
		else
			columns = 1;

		fig::coord offsetX = (_bCenterX ? (totalWidth - (columns * _cellWidth + std::max(columns - 1, 0) * _spacingX)) / 2 : 0);

		auto items = GetLayoutItems();
		int32_t index = 0;
		/*for (auto& item : items)
		{
			int col = index % columns;
			int row = index / columns;

			auto pControl = item.GetControl();
			if (pControl)
			{
				pControl->SetPosition(offsetX + col * (_cellWidth + _spacingX), row * (_cellHeight + _spacingY));
				item.rect = pControl->GetRect();
				index++;
			}
		}*/

		for (auto& item : items)
		{
			auto pControl = item.GetControl();
			auto pSizer = item.GetSizer();
			auto& info = item.info;
			int col = index % columns;
			int row = index / columns;

			fig::rect allocatedRect {
				parentRect.x + offsetX + col * (_cellWidth + _spacingX),
				parentRect.y + row * (_cellHeight + _spacingY),
				_cellWidth,
				_cellHeight
			};

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

				ClampRect(rect, item);
				AlignRect(rect, allocatedRect, item);

				OnLayoutItem(rect, item);
				pControl->SetRect(rect);
			}
			++index;
		}

		_last_columns = columns;
		_last_rows = (index % columns == 0) ? (index / columns) : (index / columns + 1);
	}

	void GridSizer::EnableCentering(bool bEnable)
	{
		_bCenterX = bEnable;
	}

	fig::point GridSizer::GetExtents() const
	{
		return fig::point {
			_last_columns * _cellWidth + _spacingX * (_last_columns - 1),
			_last_rows * _cellHeight + _spacingY * (_last_rows - 1),
		};
	}
}