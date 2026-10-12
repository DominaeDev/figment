#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class BehindChat : public Area
	{
	public:
		BehindChat(ControlPtr pParent);

		float GetAlpha() const noexcept;
		void SetAlpha(float fAlpha);

	protected:
		void SetColor(fig::color_ref color);

		fig::observer_ptr<Control> _pBG;
		fig::observer_ptr<Control> _pLeftGradient;
		fig::observer_ptr<Control> _pRightGradient;
	};
}