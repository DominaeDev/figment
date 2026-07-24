#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class TexturedBorderRenderer;
	class TexturedBorder;

	class ButtonWithLabel : public ThemedButton
	{
		ButtonWithLabel() = delete;
	public:
		ButtonWithLabel(ControlPtr pParent, const fig::string& text);

	protected:
		void OnAfterLayout() override;
		void OnButtonState() override;

	private:
		fig::observer_ptr<StaticText> _pLabel;
		fig::observer_ptr<TexturedBorder> _pBorder;
	};
}
