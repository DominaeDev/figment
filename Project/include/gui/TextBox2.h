#pragma once

#include "gui/TextInput2.h"

namespace fig::gui
{
	class TextBox2 : public TextInput2
	{
	public:
		TextBox2(ControlPtr pParent, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::TextBoxFontSize, Flags flags = {});

		void SetFixedRows(int32_t rows);

	protected:
		void OnEnabled(bool bEnabled) override;
	};
}