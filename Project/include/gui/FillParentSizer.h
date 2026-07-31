#pragma once

#include "Sizer.h"

namespace fig::gui
{
	class FillParentSizer : public Sizer
	{
	protected:
		void OnLayout(const fig::rect& rect) override;
	};
}
