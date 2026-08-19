#pragma once

#include "gui/TextInput.h"

namespace fig::gui
{
	class TextBox : public TextInput
	{
	public:
		TextBox(ControlPtr pParent, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::TextBoxFontSize, Flags flags = {});

	protected:
		void OnEnabled(bool bEnabled) override;
	};
}