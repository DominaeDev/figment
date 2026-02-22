#ifndef GRID_SIZER_H__
#define GRID_SIZER_H__
#pragma once

#include "Sizer.h"

namespace fig::gui
{
	class GridSizer : public Sizer
	{
	public:
		GridSizer(int32_t cellWidth, int32_t cellHeight);
		void SetSpacing(int32_t x, int32_t y);
		void EnableCentering(bool bEnable);

		int32_t GetSpacingX() const noexcept { return _spacingX; };
		int32_t GetSpacingY() const noexcept { return _spacingY; };
		size_t GetColumns() const noexcept { return last_columns; };
		size_t GetRows() const noexcept { return last_rows; };

	protected:
		void OnLayout(Rectf rect) override;

	private:
		int32_t _cellWidth {};
		int32_t _cellHeight {};
		int32_t _spacingX {};
		int32_t _spacingY {};
		bool _bCenterX = false;
		size_t last_columns = 0;
		size_t last_rows = 0;
		
	};
}

#endif