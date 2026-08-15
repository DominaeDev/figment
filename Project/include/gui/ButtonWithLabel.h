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
		ButtonWithLabel(ControlPtr pParent, fig::string_view label, double fontSize = 16.0);

		void SetLabel(fig::string_view label) noexcept;

	protected:
		void OnButtonState() override;
		void OnSize() override;

	protected:
		fig::observer_ptr<StaticText> _pLabel;
	};
}
