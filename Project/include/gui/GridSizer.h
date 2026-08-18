#pragma once

#include "Sizer.h"

namespace fig::gui
{
	class GridSizer : public SizerWithExtents
	{
	public:
		GridSizer(int32_t cellWidth, int32_t cellHeight, int32_t spacingX = 0, int32_t spacingY = 0);
		void SetCellSize(int32_t x, int32_t y);
		void SetSpacing(int32_t x, int32_t y);
		void EnableCentering(bool bEnable);

		int32_t GetSpacingX() const noexcept { return _spacingX; };
		int32_t GetSpacingY() const noexcept { return _spacingY; };
		size_t GetColumns() const noexcept { return _last_columns; };
		size_t GetRows() const noexcept { return _last_rows; };

		fig::point GetExtents() const override;

	protected:
		void OnLayout(const fig::rect& rect) override;

	private:
		int32_t _cellWidth {};
		int32_t _cellHeight {};
		int32_t _spacingX {};
		int32_t _spacingY {};
		bool _bCenterX = false;
		int32_t _last_columns = 0;
		int32_t _last_rows = 0;
		
	};
}
