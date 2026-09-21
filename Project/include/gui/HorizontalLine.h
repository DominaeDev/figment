#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class HorizontalLine : public Control
	{
	public:
		HorizontalLine(ControlPtr pParent, fig::color_ref_with_alpha color = Colour::LineColor);

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
	};
}