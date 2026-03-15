#pragma once

#include "Sizer.h"

namespace fig::gui
{
	class HorizontalSizer : public Sizer
	{
	protected:
		void OnLayout(Rect rect) override;
	};
}