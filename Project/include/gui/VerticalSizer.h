#pragma once

#include "Sizer.h"

namespace fig::gui
{
	class VerticalSizer : public Sizer
	{
	protected:
		void OnLayout(Rectf rect) override;
	};
}