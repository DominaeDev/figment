#pragma once

#include "Control.h"

namespace fig::gui
{
	class BehindChat : public Area
	{
	public:
		BehindChat(ControlPtr pParent);

		void SetColor(fig::color color);
		void SetColor(fig::color color, float fAlpha);
		void SetAlpha(float fAlpha);

	protected:
		fig::observer_ptr<Control> _pBG;
		fig::observer_ptr<Control> _pLeftGradient;
		fig::observer_ptr<Control> _pRightGradient;
	};
}