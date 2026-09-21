#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class HorizontalLine : public Control
	{
	public:
		HorizontalLine(ControlPtr pParent, fig::color_ref color = Colour::LineColor);

		void SetColor(fig::color_ref color) { _color = color; };

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;

	private:
		fig::color_ref _color;
	};
}