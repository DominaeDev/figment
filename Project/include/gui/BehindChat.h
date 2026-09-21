#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class BehindChat : public Area
	{
	public:
		BehindChat(ControlPtr pParent);

		void SetColor(fig::color_ref_with_alpha color);
		void SetAlpha(float fAlpha);

	protected:
		fig::observer_ptr<Control> _pBG;
		fig::observer_ptr<Control> _pLeftGradient;
		fig::observer_ptr<Control> _pRightGradient;
	};
}