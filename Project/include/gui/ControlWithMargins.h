#pragma once

#include "Control.h"

namespace fig::gui
{
	class ControlWithMargins : public Control
	{
	public:
		ControlWithMargins(LayoutElement* pParent);

		void SetMargins(float left, float top, float right, float bottom);
		void SetMargins(Rectf rect);
		Rectf GetClientRect() const noexcept;
		inline float GetMarginLeft() const noexcept { return _marginLeft; }
		inline float GetMarginTop() const noexcept { return _marginTop; }
		inline float GetMarginRight() const noexcept { return _marginRight; }
		inline float GetMarginBottom() const noexcept { return _marginBottom; }

	protected:
		float GetMarginHorizontal() const { return _marginLeft + _marginRight; }
		float GetMarginVertical() const { return _marginTop + _marginBottom; }
	
	private:
		float _marginLeft = 0;
		float _marginTop = 0;
		float _marginRight = 0;
		float _marginBottom = 0;
	};
}