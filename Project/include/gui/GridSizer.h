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

	protected:
		void OnLayout(Rectf rect) override;

	private:
		int32_t _cellWidth {};
		int32_t _cellHeight {};
		int32_t _spacingX {};
		int32_t _spacingY {};
		bool _bCenterX = false;
		
	};
}

#endif