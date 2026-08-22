#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class HorizontalLine : public Control
	{
	public:
		HorizontalLine(ControlPtr pParent, fig::color color = Color::LineColor);

		void SetColor(const fig::color& color) { _color = color; };

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;

	private:
		fig::color _color;
	};
}