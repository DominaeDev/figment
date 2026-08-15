#pragma once

#include "gui/TextBox.h"

namespace fig::gui
{
	class SimpleTextBox : public TextBox
	{
	public:
		SimpleTextBox(ControlPtr pParent, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::TextBoxFontSize, Flags flags = {});

	protected:
		void OnEnabled(bool bEnabled) override;
	};
}