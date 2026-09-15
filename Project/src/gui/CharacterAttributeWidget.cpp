#include <pch.h>
#include "gui/CharacterAttributeWidget.h"
#include "gui/TextBox.h"
#include "gui/TextInput.h"
#include "gui/ComboBox.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"

using namespace fig::data;

namespace fig::gui
{
	constexpr fig::coord MaxWidth = 780;

	CharacterAttributeWidget::CharacterAttributeWidget(ControlPtr pParent, fig::string_view label, fig::string_view value, CharacterAttribute::ValueType type, const fig::string_list& options, fig::string_view placeholder) : Control(pParent),
		_options { options }
	{
		_pLabel = CreateControl<StaticText>(label, FontFace::Default, 14.0, false);
		_pLabel->SetX(4);
		_pLabel->SetForegroundColor(Color::SidePanelForeground);

		_pSettingsButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_SETTINGS);
		_pSettingsButton->SetSize(20, 20);

		InitValue(value, type);

		if (_pTextBox)
			_pTextBox->SetPlaceholder(placeholder);
	}

	void CharacterAttributeWidget::SetButtonDelegate(MouseClickedDelegate fnDelegate)
	{
		_pSettingsButton->SetDelegate(fnDelegate);
	}

	void CharacterAttributeWidget::ChangeType(fig::data::CharacterAttribute::ValueType type)
	{
		InitValue("", type);
		_options.clear();
	}

	void CharacterAttributeWidget::InitValue(fig::string_view value, fig::data::CharacterAttribute::ValueType type)
	{
		if (_pTextBox)
			value = _pTextBox->GetText();
		else if (_pComboBox)
			value = _pComboBox->GetText();

		if (not _options.empty())
		{
			_pComboBox = CreateControl<ComboBox>();
			_pComboBox->SetPosition(0, 23);
			_pComboBox->AddItems(_options);
			_pComboBox->SetText(value);
			SetHeight(_pComboBox->GetY() + _pComboBox->GetHeight());
			return;
		}

		if (_pComboBox)
		{
			DestroyChild(_pComboBox);
			_pComboBox = nullptr;
		}

		if (not _pTextBox)
		{
			_pTextBox = CreateControl<TextBox>(FontFace::Default, 14.0);
			_pTextBox->SetPosition(0, 23);
			_pTextBox->EnableAutoSize(true);
		}

		fig::string_view value_sv = value;

		auto fnTrimLine = [](fig::string_view text) {
			size_t newlinePos = text.find('\n', 0);
			return fig::string_view { text.data(), std::min(text.length(), newlinePos) };
		};

		switch (type)
		{
		case CharacterAttribute::ValueType::ShortText:
			_pTextBox->SetMode(TextInput::Mode::SingleWordWrap);
			_pTextBox->SetTextWrapWidth(_pTextBox->GetClientRect().w);
			_pTextBox->SetMinRows(1);
			_pTextBox->SetMaxRows(4);
			_pTextBox->SetMaxWidth(MaxWidth);
			_pTextBox->SetWidth(MaxWidth);
			value_sv = fnTrimLine(value);
			break;
		case CharacterAttribute::ValueType::List:
			_pTextBox->SetMode(TextInput::Mode::Single);
			_pTextBox->SetTextWrapWidth(0);
			_pTextBox->SetMinRows(1);
			_pTextBox->SetMaxRows(1);
			_pTextBox->SetMaxWidth(MaxWidth);
			_pTextBox->SetWidth(MaxWidth);
			value_sv = fnTrimLine(value);
			break;
		case CharacterAttribute::ValueType::Number:
			_pTextBox->SetMode(TextInput::Mode::Single);
			_pTextBox->SetTextWrapWidth(0);
			_pTextBox->SetMinRows(1);
			_pTextBox->SetMaxRows(1);
			_pTextBox->SetMaxWidth(120);
			_pTextBox->SetWidth(120);
			value_sv = fnTrimLine(value);
			break;
		case CharacterAttribute::ValueType::LongText:
			_pTextBox->SetMode(TextInput::Mode::Multiline);
			_pTextBox->SetTextWrapWidth(_pTextBox->GetClientRect().w);
			_pTextBox->SetMinRows(2);
			_pTextBox->SetMaxRows(12);
			_pTextBox->SetMaxWidth(MaxWidth);
			_pTextBox->SetWidth(MaxWidth);
			break;
		}

		_pTextBox->SetText(value_sv);
		_pTextBox->SetCursor(0uz);
		_pTextBox->SetPlaceholder("");

		SetHeight(_pTextBox->GetY() + _pTextBox->GetHeight());
	}

	void CharacterAttributeWidget::OnUpdate(float)
	{
		if (_pTextBox and _pTextBox->GetHeight() != _lastTextBoxHeight)
		{
			_lastTextBoxHeight = _pTextBox->GetHeight();
			SetHeight(_pTextBox->GetY() + _pTextBox->GetHeight());
			InvalidateParentLayout();
		}
	}

	void CharacterAttributeWidget::OnSize()
	{
		if (_pTextBox)
			_pTextBox->SetWidth(GetWidth());
		
		if (_pSettingsButton)
			_pSettingsButton->SetX(std::min(GetWidth(), MaxWidth) - _pSettingsButton->GetWidth());
	}

	fig::string_view CharacterAttributeWidget::GetValue() const noexcept
	{
		if (_pTextBox)
			return _pTextBox->GetText();
		else if (_pComboBox)
			return _pComboBox->GetText();
		else
		{
			static fig::string EmptyValue {};
			return EmptyValue;
		}
	}

	void CharacterAttributeWidget::Focus()
	{
		if (_pTextBox)
			return _pTextBox->SetFocus(true);
		else if (_pComboBox)
			return _pComboBox->SetFocus(true);
	}

}