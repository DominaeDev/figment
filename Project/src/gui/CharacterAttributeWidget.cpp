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

	CharacterAttributeWidget::CharacterAttributeWidget(ControlPtr pParent, fig::string_view label, fig::string_view value, CharacterAttribute::ValueType type, const fig::string_list& options, fig::string_view placeholder) : Control(pParent), MouseEventHandler(this),
		_options { options }
	{
		_pLabel = CreateControl<StaticText>(label, FontFace::Default, 14.0, true);
		_pLabel->SetX(4);
		_pLabel->SetForegroundColor(Color::SidePanelForeground);

		_pEditLabel = CreateControl<TextInput>(FontFace::Default, 14.0, TextInput::Mode::Single);
		_pEditLabel->SetEnterPressedDelegate([this](auto&& text) { EndEditName(); });
		_pEditLabel->SetEscapePressedDelegate([this] { CancelEditName(); });
		_pEditLabel->SetLostFocusDelegate([this] { EndEditName(); });
		_pEditLabel->SetEnabled(false);
		_pEditLabel->SetVisible(false);

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

		auto value_sv = fig::string { value };

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
		{
			return _pTextBox->SetFocus(true);
		}
		else if (_pComboBox)
		{
			return _pComboBox->SetFocus(true);
		}
	}

	void CharacterAttributeWidget::SetEditNameDelegate(EditNameDelegate fnDelegate)
	{
		_fnRenameDelegate = fnDelegate;
	}

	void CharacterAttributeWidget::BeginEditName()
	{
		if (_bRenaming)
			return;
		_bRenaming = true;

		_pLabel->SetVisible(false);
	
		_pEditLabel->SetPosition(_pLabel->GetPosition());
		_pEditLabel->SetWidth(260);
		_pEditLabel->SetBackgroundColor(GetBackgroundColor());
		_pEditLabel->SetText(_pLabel->GetText());
		_pEditLabel->SetVisible(true);
		_pEditLabel->SetEnabled(true);
		_pEditLabel->SelectAll();
		_pEditLabel->SetFocus(true);
	}

	void CharacterAttributeWidget::EndEditName()
	{
		if (not _bRenaming)
			return;
		_bRenaming = false;

		auto name = trim(_pEditLabel->GetText());
		if (name.empty())
			name = "Unnamed attribute";

		_pLabel->SetText(name);
		_pLabel->SetVisible(true);
		if (_fnRenameDelegate)
			_fnRenameDelegate(fig::string { name });

		_pEditLabel->SetVisible(false);
		_pEditLabel->SetEnabled(false);
	}

	void CharacterAttributeWidget::CancelEditName()
	{
		_bRenaming = false;
		_pLabel->SetVisible(true);
		_pEditLabel->SetVisible(false);
		_pEditLabel->SetEnabled(false);
		_pEditLabel->SetFocus(false);
	}

	EventResult CharacterAttributeWidget::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN)
		{
			if (event.key.key == SDLK_ESCAPE and _bRenaming)
			{
				CancelEditName();
				return EventResult::Handled;
			}
		}

		return MouseEventHandler::HandleMouseEvents(event);
	}

	void CharacterAttributeWidget::OnDoubleClickedAt(fig::point pos)
	{
		if (is_inside(_pLabel->GetRect(), pos))
			BeginEditName();
	}
}