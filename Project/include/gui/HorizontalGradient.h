#pragma once

#include "gui/IMeshControl.h"

namespace fig::gui
{
	class HorizontalGradient : public IMeshControl
	{
	public:
		HorizontalGradient(ControlPtr pParent, fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight);
		void SetColors(fig::color_ref_with_alpha colorLeft, fig::color_ref_with_alpha colorRight);

	protected:
		void RefreshGeometry(const fig::rectf& rect) override;

	private:
		fig::color_ref_with_alpha _colorLeft {};
		fig::color_ref_with_alpha _colorRight {};
	};
}