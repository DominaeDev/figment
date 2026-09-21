#pragma once

#include "gui/IMeshControl.h"

namespace fig::gui
{
	class VerticalGradient : public IMeshControl
	{
	public:
		VerticalGradient(ControlPtr pParent, fig::color_ref_with_alpha colorTop, fig::color_ref_with_alpha colorBottom);
		void SetColors(fig::color_ref_with_alpha colorTop, fig::color_ref_with_alpha colorBottom);

	protected:
		void RefreshGeometry(const fig::rectf& rect) override;

	private:
		fig::color_ref_with_alpha _colorTop {};
		fig::color_ref_with_alpha _colorBottom {};
	};
}