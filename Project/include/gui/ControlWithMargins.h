#pragma once

#include "Control.h"

namespace fig::gui
{
	class ControlWithMargins : public Control
	{
	public:
		ControlWithMargins(Control* pParent);

		void SetMargins(float left, float top, float right, float bottom);
		void SetMargins(Rectf rect);

		Rectf GetClientRect() const;

	protected:
		float _marginLeft = 0;
		float _marginTop = 0;
		float _marginRight = 0;
		float _marginBottom = 0;

		float HMargin() const { return _marginLeft + _marginRight; }
		float VMargin() const { return _marginTop + _marginBottom; }
	};
}