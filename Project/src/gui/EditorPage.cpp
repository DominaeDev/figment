#include <pch.h>
#include "gui/EditorPage.h"
#include "gui/TextBox.h"

namespace fig::gui
{
	fig::observer_ptr<StaticText> EditorPage::CreateHeader(ControlPtr pParent, SizerPtr pSizer, fig::string_view text)
	{
		auto pLabel = pParent->CreateControl<StaticText>(fig::string { text }, FontFace::Default, 18.5, false);
		pParent->GetSizer()->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Top, 8);
		pSizer->AddSpacer(9);
		return pLabel;
	}

	fig::observer_ptr<StaticText> EditorPage::CreateLabel(ControlPtr pParent, SizerPtr pSizer, fig::string_view text)
	{
		auto pLabel = pParent->CreateControl<StaticText>(fig::string { text }, FontFace::Default, 14.0, false);
		pLabel->SetForegroundColor(Color::SidePanelForeground);
		pSizer->AddSpacer(8);
		pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left, 4);
		pSizer->AddSpacer(3);
		return pLabel;
	}

	fig::observer_ptr<StaticText> EditorPage::CreateHint(ControlPtr pParent, SizerPtr pSizer, fig::string_view text)
	{
		auto pLabel = pParent->CreateControl<StaticText>(fig::string { text }, FontFace::Italic, 14.0, false);
		pLabel->SetForegroundColor(0x8a8375_rgb);
		pSizer->AddSpacer(2);
		pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left, 2);
		pSizer->AddSpacer(3);
		return pLabel;
	}

	template <>
	fig::observer_ptr<TextBox> EditorPage::CreateTextBox<fig::string>(ControlPtr pParent, SizerPtr pSizer, ValueBinding<fig::string> binding, int32_t rows)
	{
		auto pTextBox = pParent->CreateControl<TextBox>(FontFace::Default, rows == 1 ? Constants::GUI::TextBoxFontSize : 14.5, rows > 1 ? TextInput::Mode::Multiline : TextInput::Mode::Single );

		pTextBox->SetText(binding.AsString());
		pTextBox->SetFixedRows(rows);
		pTextBox->SetTextChangedCallback([binding](fig::string_view text) mutable { binding.Set(fig::string { text }); });
		pSizer->Add(pTextBox, 0, SizerFlag::Expand, 0);
		return pTextBox;
	}
	
}