#pragma once

#include "gui/IMeshControl.h"

namespace fig::gui
{
	class HorizontalGradient : public IMeshControl
	{
	public:
		HorizontalGradient(ControlPtr pParent, fig::color_ref colorLeft, fig::color_ref colorRight);
		void SetColors(fig::color_ref colorLeft, fig::color_ref colorRight);

	protected:
		void RefreshGeometry(const fig::rectf& rect) override;

	private:
		fig::color_ref _colorLeft {};
		fig::color_ref _colorRight {};
	};
}