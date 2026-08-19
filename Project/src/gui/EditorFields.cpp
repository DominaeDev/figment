#include <pch.h>
#include "gui/EditorFields.h"
#include "gui/TextBox.h"

namespace fig::gui
{
	template<>
	ControlPtr EditorTextField<fig::string>::CreateControl(ControlPtr pParent)
	{
		_pTextBox = pParent->CreateControl<TextBox>(FontFace::Default, Constants::GUI::TextBoxFontSize, TextInput::Flags { TextInput::Flag::Single });
		_pTextBox->SetText(_binding.AsString());
		_pTextBox->SetMaxWidth(_maxWidth);
		_pTextBox->SetTextChangedCallback([this](const fig::string& text) { OnChange(text); });
		return _pTextBox;
	}

	template<>
	void EditorTextField<fig::string>::OnChange(const fig::string& text)
	{
		_binding.Set(text);
	}
}