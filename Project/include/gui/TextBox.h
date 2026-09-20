#pragma once

#include "gui/TextInput.h"

namespace fig::gui
{
	class TextBox : public TextInput
	{
	public:
		TextBox(ControlPtr pParent, FontFace fontFace = FontFace::Default, double ptSize = Constants::GUI::TextBoxFontSize, TextInput::Mode mode = Mode::Single);

		void SetFixedRows(int32_t rows);

	protected:
		void OnEnabled(bool bEnabled) override;
		void OnUpdate(float) override;
		void OnSize() override;

		void RefreshScrollBar();
		fig::observer_ptr<class VerticalBar> _pScrollBar;
	};
}