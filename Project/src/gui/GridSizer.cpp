#include <pch.h>
#include "gui/GridSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::util;

namespace fig::gui
{
	GridSizer::GridSizer(int32_t cellWidth, int32_t cellHeight) :
		_cellWidth { cellWidth },
		_cellHeight { cellHeight }
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

	void GridSizer::OnLayout(Rectf parentRect)
	{
		auto count = GetCount();
		if (count == 0)
		{
			last_columns = 0;
			last_rows = 0;
			return;
		}

		int totalWidth = ceil_int(std::max(parentRect.w, 0.0f));
		int columns;
		if (totalWidth > 0 && _cellWidth > 0)
			columns = std::max((totalWidth + _spacingX) / toI(_cellWidth + _spacingX), 1);
		else
			columns = 1;

		float offsetX = _bCenterX ? toF(totalWidth - (columns * _cellWidth + std::max(columns - 1, 0) * _spacingX)) / 2 : 0.0f;

		int32_t index = 0;
		for (auto& item : GetLayoutItems())
		{
			int col = index % columns;
			int row = index / columns;

			auto& control = *item.pControl;

			control.SetPosition(offsetX + toF(col * (_cellWidth + _spacingX)), toF(row * (_cellHeight + _spacingY)));
			index++;

		}

		last_columns = columns;
		last_rows = (index % columns == 0) ? (index / columns) : (index / columns + 1);
	}

	void GridSizer::EnableCentering(bool bEnable)
	{
		_bCenterX = bEnable;
	}
}