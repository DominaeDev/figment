#pragma once

#include "gui/Control.h"
#include "gui/ThemedButton.h"

namespace fig::gui
{
	class TexturedBorderRenderer;
	class TexturedBorder;

	class ButtonWithLabelAndIcon : public ThemedButton
	{
		ButtonWithLabelAndIcon() = delete;
	public:
		ButtonWithLabelAndIcon(ControlPtr pParent, fig::string_view label, Resource icon, double fontSize = 16.0);

		void SetLabel(fig::string_view label) noexcept;
		void SetIcon(Resource icon);

	protected:
		void OnButtonState() override;
		void OnSize() override;

	protected:
		fig::observer_ptr<StaticText> _pLabel;
		fig::observer_ptr<Image> _pIcon;
	};
}
