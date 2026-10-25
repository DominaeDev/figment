#include <pch.h>
#include "gui/CharacterDialogueWidget.h"
#include "gui/TextBox.h"
#include "gui/TextInput.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithIcon.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"
#include "gui/DefaultTextStyleProvider.h"

using namespace fig::chat;
using namespace fig::data;

namespace fig::gui
{
	constexpr fig::coord MaxWidth = 620;

	CharacterDialogueWidget::CharacterDialogueWidget(ControlPtr pParent, const ExampleDialogue& example, fig::observer_ptr<const Character> pCharacter) : Control(pParent), MouseEventHandler(this),
		_pCharacter { pCharacter }
	{
		auto pSizer = SetSizer<VerticalSizer>();

		_pInputLabel = CreateControl<StaticText>("You:", FontFace::Default, 14.0, true);
		_pInputLabel->SetPosition(4, 0);
		_pInputLabel->SetForegroundColor(Color::SidePanelForeground);
		pSizer->Add(_pInputLabel, 0, SizerFlag::Expand | SizerFlag::Left, 6);

		_pInput = CreateControl<TextBox>(FontFace::Default, 14.0);
		_pInput->SetStyleProvider<DefaultTextStyleProvider>();
		_pInput->SetTextChangedDelegate([this](auto&& value) { OnValueChanged(); });
		_pInput->EnableAutoSize(true);
		_pInput->SetMode(TextInput::Mode::Multiline);
		_pInput->SetMinRows(1);
		_pInput->SetMaxRows(6);
		_pInput->SetMaxWidth(MaxWidth);
		_pInput->SetWidth(MaxWidth);
		_pInput->SetText(example.input);
		_pInput->SetPlaceholder("Enter something the user might say");
		pSizer->Add(_pInput, 0, SizerFlag::Expand | SizerFlag::Bottom, 4);

		_pResponseLabel = CreateControl<StaticText>(std::format("{}:", pCharacter->name.GetSpokenName()), FontFace::Default, 14.0, true);
		_pResponseLabel->SetPosition(4, _pInput->GetBottom() + 6);
		_pResponseLabel->SetForegroundColor(Color::SidePanelForeground);
		pSizer->Add(_pResponseLabel, 0, SizerFlag::Expand | SizerFlag::Left, 6);

		_pResponse = CreateControl<TextBox>(FontFace::Default, 14.0);
		_pResponse->SetStyleProvider<DefaultTextStyleProvider>();
		_pResponse->SetTextChangedDelegate([this](auto&& value) { OnValueChanged(); });
		_pResponse->EnableAutoSize(true);
		_pResponse->SetMode(TextInput::Mode::Multiline);
		_pResponse->SetMinRows(1);
		_pResponse->SetMaxRows(6);
		_pResponse->SetMaxWidth(MaxWidth);
		_pResponse->SetWidth(MaxWidth);
		_pResponse->SetText(example.response);
		_pResponse->SetPlaceholder("Enter the character's response");
		pSizer->Add(_pResponse, 0, SizerFlag::Expand | SizerFlag::Bottom, 6);

		_pSettingsButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_SETTINGS);
		_pSettingsButton->SetSize(20, 20);
		_pMoveUpButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_MOVE_UP);
		_pMoveUpButton->SetSize(20, 20);
		_pMoveUpButton->SetDelegate([this] { OnMove(-1); });
		_pMoveDownButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_MOVE_DOWN);
		_pMoveDownButton->SetSize(20, 20);
		_pMoveDownButton->SetDelegate([this] { OnMove(1); });

		SetHeight(_pResponse->GetBottom());
		CreateTabGroup();
	}

	void CharacterDialogueWidget::SetButtonDelegate(MouseClickedDelegate fnDelegate)
	{
		_pSettingsButton->SetDelegate(fnDelegate);
	}

	void CharacterDialogueWidget::SetMoveDelegate(MoveDelegate fnDelegate)
	{
		_fnMoveDelegate = fnDelegate;
	}

	fig::data::ExampleDialogue CharacterDialogueWidget::GetValue() const noexcept
	{
		return fig::data::ExampleDialogue {
			.input = fig::string { _pInput->GetText() },
			.response = fig::string { _pResponse->GetText() },
		};
	}

	void CharacterDialogueWidget::SetValueChangedDelegate(ValueChangedDelegate fnDelegate)
	{
		_fnChangedDelegate = fnDelegate;
	}

	void CharacterDialogueWidget::OnUpdate(float)
	{
		if (_pResponse and _pResponse->GetBottom() != _lastTextBoxHeight)
		{
			_lastTextBoxHeight = _pResponse->GetHeight();
			SetHeight(_pResponse->GetBottom());
			InvalidateParentLayout();
		}
	}

	void CharacterDialogueWidget::OnSize()
	{
		if (_pSettingsButton)
			_pSettingsButton->SetX(std::min(GetWidth(), MaxWidth) - 20);
		if (_pMoveDownButton)
			_pMoveDownButton->SetX(std::min(GetWidth(), MaxWidth) - 42);
		if (_pMoveUpButton)
			_pMoveUpButton->SetX(std::min(GetWidth(), MaxWidth) - 64);
	}

	EventResult CharacterDialogueWidget::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}

	void CharacterDialogueWidget::OnMove(int32_t dir) noexcept
	{
		if (_fnMoveDelegate)
			_fnMoveDelegate(dir);
	}

	void CharacterDialogueWidget::OnValueChanged() noexcept
	{
		if (_fnChangedDelegate)
			_fnChangedDelegate(GetValue());
	}

	fig::point CharacterDialogueWidget::GetPreferredSize() const noexcept
	{
		if (_pResponse)
			return fig::point { GetWidth(), _pResponse->GetBottom() + 8 };
		return GetSize();
	}

	void CharacterDialogueWidget::SetFocus(bool bFocus, TabDirection dir)
	{
		if (bFocus)
		{
			if (dir == TabDirectionForward)
				_pInput->SetFocus(true);
			else
				_pResponse->SetFocus(true);
		}
		else
		{
			_pInput->SetFocus(false);
			_pResponse->SetFocus(false);
		}
	}

	bool CharacterDialogueWidget::IsFocused() const
	{
		return (_pInput and _pInput->IsFocused()) or (_pResponse and _pResponse->IsFocused());
	}

}