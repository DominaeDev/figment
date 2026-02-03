#include <pch.h>
#include "gui/GridSizer.h"
#include "gui/Control.h"
#include "util/Common.h"

using namespace fig::common_util;

namespace fig::gui
{
	GridSizer::GridSizer(int32_t cellWidth, int32_t cellHeight) :
		_cellWidth { cellWidth },
		_cellHeight { cellHeight }
	{
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
			return;

		int totalWidth = ceil_int(std::max(parentRect.w, 0.0f));
		int columns;
		if (totalWidth > 0 && _cellWidth > 0)
			columns = std::max((totalWidth + _spacingX) / toI(_cellWidth + _spacingX), 1);
		else
			columns = 1;

		int rows = (count % columns == 0) ? (count / columns) : (count / columns + 1);

		int32_t index = 0;
		for (auto& item : _items)
		{
			if (item.pControl == nullptr)
				continue;
			int col = index % columns;
			int row = index / columns;

			auto& control = *item.pControl;

			control.SetPosition(toF(col * (_cellWidth + _spacingX)), toF(row * (_cellHeight + _spacingY)));
			index++;
		}
	}
}