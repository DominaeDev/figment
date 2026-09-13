#include <pch.h>
#include "gui/CharacterAttributeWidget.h"
#include "gui/TextBox.h"
#include "gui/TextInput.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"

using namespace fig::data;

namespace fig::gui
{
	constexpr fig::coord MaxWidth = 780;

	CharacterAttributeWidget::CharacterAttributeWidget(ControlPtr pParent, CharacterAttribute& attribute) : Control(pParent),
		_pAttribute { &attribute }
	{
		if (attribute.name == "Persona") //! @temp
			attribute.format = CharacterAttribute::ValueType::LongText;

		_pLabel = CreateControl<StaticText>(attribute.name, FontFace::Default, 14.0, false);
		_pLabel->SetX(4);
		_pLabel->SetForegroundColor(Color::SidePanelForeground);

		_pSettingsButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_SETTINGS);
		_pSettingsButton->SetSize(20, 20);
		_pSettingsButton->SetDelegate([this] { ShowMenu(); });

		InitValue();
	}

	void CharacterAttributeWidget::InitValue()
	{
		if (not (bool)_pAttribute)
			return;

		if (not (bool)_pTextBox)
		{
			_pTextBox = CreateControl<TextBox>(FontFace::Default, 14.0);
			_pTextBox->SetPosition(0, 23);
			_pTextBox->SetMaxWidth(MaxWidth);
			_pTextBox->EnableAutoSize(true);
		}

		fig::string_view text = _pAttribute->value;

		switch (_pAttribute->format)
		{
		case CharacterAttribute::ValueType::ShortText:
			_pTextBox->SetMode(TextInput::Mode::SingleWordWrap);
			_pTextBox->SetWidth(300);
			_pTextBox->SetTextWrapWidth(_pTextBox->GetClientRect().w);
			_pTextBox->SetMinRows(1);
			_pTextBox->SetMaxRows(4);
			break;
		case CharacterAttribute::ValueType::List:
			_pTextBox->SetMode(TextInput::Mode::Single);
			_pTextBox->SetWidth(MaxWidth);
			_pTextBox->SetTextWrapWidth(0);
			_pTextBox->SetMinRows(1);
			_pTextBox->SetMaxRows(1);
			break;
		case CharacterAttribute::ValueType::Number:
			_pTextBox->SetMode(TextInput::Mode::Single);
			_pTextBox->SetWidth(120);
			_pTextBox->SetTextWrapWidth(0);
			_pTextBox->SetMinRows(1);
			_pTextBox->SetMaxRows(1);
			break;
		case CharacterAttribute::ValueType::LongText:
			_pTextBox->SetMode(TextInput::Mode::Multiline);
			_pTextBox->SetWidth(MaxWidth);
			_pTextBox->SetTextWrapWidth(_pTextBox->GetClientRect().w);
			_pTextBox->SetMinRows(2);
			_pTextBox->SetMaxRows(12);
			break;
		}

		_pTextBox->SetText(text);
		_pTextBox->SetCursor(0uz);

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
		_pTextBox->SetWidth(GetWidth());
		_pSettingsButton->SetX(std::min(GetWidth(), MaxWidth) - _pSettingsButton->GetWidth());
	}

	void CharacterAttributeWidget::ShowMenu()
	{
		if (not (bool)_pAttribute)
			return;

		auto& menu = CreateMenu();
		auto& typeMenu = menu.AddItem("Value type");
		typeMenu.AddCheckItem("Text (short)", _pAttribute->format == CharacterAttribute::ValueType::ShortText)
			.SetDelegate([this] { 
				_pAttribute->format = CharacterAttribute::ValueType::ShortText;
				InitValue();
			});
		typeMenu.AddCheckItem("Text (long)", _pAttribute->format == CharacterAttribute::ValueType::LongText)
			.SetDelegate([this] { 
				_pAttribute->format = CharacterAttribute::ValueType::LongText; 
				InitValue();
			});
		typeMenu.AddCheckItem("Number", _pAttribute->format == CharacterAttribute::ValueType::Number)
			.SetDelegate([this] { 
				_pAttribute->format = CharacterAttribute::ValueType::Number; 
				InitValue();
			});
		typeMenu.AddCheckItem("Comma-separated list", _pAttribute->format == CharacterAttribute::ValueType::List)
			.SetDelegate([this] { 
				_pAttribute->format = CharacterAttribute::ValueType::List; 
				InitValue();
			});

		auto& visibilityMenu = menu.AddItem("Visibility");
		visibilityMenu.AddCheckItem("Public", _pAttribute->visibility == CharacterAttribute::Visibility::Public)
			.SetDelegate([this] { _pAttribute->visibility = CharacterAttribute::Visibility::Public; });
		visibilityMenu.AddCheckItem("Private", _pAttribute->visibility == CharacterAttribute::Visibility::Private)
			.SetDelegate([this] { _pAttribute->visibility = CharacterAttribute::Visibility::Private; });

		auto& priorityMenu = menu.AddItem("Priority");
		priorityMenu.AddCheckItem("Trivial", _pAttribute->flags.IsSet(CharacterAttribute::HintFlag::Trivial))
			.SetDelegate([this] { _pAttribute->flags.Flip(CharacterAttribute::HintFlag::Trivial); _pAttribute->flags.Unset(CharacterAttribute::HintFlag::Important); });
		priorityMenu.AddCheckItem("Important", _pAttribute->flags.IsSet(CharacterAttribute::HintFlag::Important))
			.SetDelegate([this] { _pAttribute->flags.Flip(CharacterAttribute::HintFlag::Important); _pAttribute->flags.Unset(CharacterAttribute::HintFlag::Trivial); });

		menu.AddSeparator();
		menu.AddItem("Rename ...");
		menu.AddSeparator();
		menu.AddItem("Copy");
		menu.AddItem("Paste");
		menu.AddSeparator();
		menu.AddItem("Move to top");
		menu.AddItem("Move up");
		menu.AddItem("Move down");
		menu.AddItem("Move to bottom");

		menu.AddSeparator();
		menu.AddItem("Delete");

		menu.Show();
	}
}