#include <pch.h>
#include "gui/StoryEventWidget.h"
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

	StoryEventWidget::StoryEventWidget(ControlPtr pParent, const Scenario::Event& storyEvent, fig::observer_ptr<const Scenario> pScenario) : Control(pParent), MouseEventHandler(this),
		_pScenario { pScenario },
		_eventType { storyEvent.type }
	{
		_pLabel = CreateControl<StaticText>("", FontFace::Default, 14.0, true);
		_pLabel->SetX(4);
		_pLabel->SetForegroundColor(Color::SidePanelForeground);

		_pSettingsButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_SETTINGS);
		_pSettingsButton->SetSize(20, 20);
		_pMoveUpButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_MOVE_UP);
		_pMoveUpButton->SetSize(20, 20);
		_pMoveUpButton->SetDelegate([this] { OnMove(-1); });
		_pMoveDownButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_MOVE_DOWN);
		_pMoveDownButton->SetSize(20, 20);
		_pMoveDownButton->SetDelegate([this] { OnMove(1); });

		InitValue(storyEvent);
	}

	void StoryEventWidget::SetButtonDelegate(MouseClickedDelegate fnDelegate)
	{
		_pSettingsButton->SetDelegate(fnDelegate);
	}

	void StoryEventWidget::SetMoveDelegate(MoveDelegate fnDelegate)
	{
		_fnMoveDelegate = fnDelegate;
	}

	fig::data::Scenario::Event StoryEventWidget::GetValue() const noexcept
	{
		return fig::data::Scenario::Event {
			.type = _eventType,
			.role = GetRole(),
			.content = fig::string { GetContent() },
		};
	}

	void StoryEventWidget::SetValueChangedDelegate(ValueChangedDelegate fnDelegate)
	{
		_fnChangedDelegate = fnDelegate;
	}

	void StoryEventWidget::CreateRoleDropList(Role role)
	{
		_pRoleDropList = CreateControl<DropListOfType<Role>>();
		_pRoleDropList->SetWidth(240);
		_pRoleDropList->SetDelegate([this](auto&& value) { OnValueChanged(); });
		_pRoleDropList->SetPosition(0, 23);

		for (auto& role : _pScenario->roles)
		{
			_pRoleDropList->AddItem(role.label, role.role);
		}
		_pRoleDropList->AddItem("Narrator", Role::Narrator);
		_pRoleDropList->SelectValue(role);
		SetHeight(_pRoleDropList->GetY() + _pRoleDropList->GetHeight());
	}

	void StoryEventWidget::CreateMessageTypeDropList(MessageType msgType)
	{
		_pMsgTypeDropList = CreateControl<DropListOfType<MessageType>>();
		_pMsgTypeDropList->SetWidth(240);
		_pMsgTypeDropList->SetDelegate([this](auto&& value) { OnValueChanged(); });
		_pMsgTypeDropList->SetPosition(248, 23);

		if (msgType == MessageType::Undefined)
			msgType = MessageType::Dialogue;

		_pMsgTypeDropList->AddItem("Dialogue", MessageType::Dialogue);
		_pMsgTypeDropList->AddItem("Action", MessageType::Action);
		_pMsgTypeDropList->AddItem("Internal thought", MessageType::Thought);
		_pMsgTypeDropList->SelectValue(msgType);
		SetHeight(_pMsgTypeDropList->GetY() + _pMsgTypeDropList->GetHeight());
	}

	void StoryEventWidget::CreateContentTextBox(fig::string_view text, fig::string_view placeholder, bool bMultiline)
	{
		_pTextBox = CreateControl<TextBox>(FontFace::Default, 14.0);
		_pTextBox->SetStyleProvider<DefaultTextStyleProvider>();
		_pTextBox->SetTextChangedDelegate([this](auto&& value) { OnValueChanged(); });
		_pTextBox->SetPosition(0, _pRoleDropList ? _pRoleDropList->GetBottom() + 6 : 23);
		_pTextBox->EnableAutoSize(true);
		_pTextBox->SetMode(bMultiline ? TextInput::Mode::Multiline : TextInput::Mode::SingleWordWrap);
		_pTextBox->SetMinRows(bMultiline ? 2 : 1);
		_pTextBox->SetMaxRows(bMultiline ? 12 : 4);
		_pTextBox->SetMaxWidth(MaxWidth);
		_pTextBox->SetWidth(MaxWidth);
		_pTextBox->SetText(text);
		_pTextBox->SetPlaceholder(placeholder);
	}

	void StoryEventWidget::InitValue(const Scenario::Event& storyEvent)
	{
		if (storyEvent.type == Scenario::Event::Type::ChatMessage)
		{
			_pLabel->SetText("Chat message from");

			CreateRoleDropList(storyEvent.role);
			CreateMessageTypeDropList(storyEvent.msgType);
			CreateContentTextBox(storyEvent.content, "Type a chat message", true);
		}
		else if (storyEvent.type == Scenario::Event::Type::Instigate)
		{
			_pLabel->SetText("Instigate response from");
			CreateRoleDropList(storyEvent.role);
		}
		else if (storyEvent.type == Scenario::Event::Type::Instruct)
		{
			_pLabel->SetText("Give instruction");
			CreateContentTextBox(storyEvent.content, "Type an instruction for the AI to follow", false);
		}
		else if (storyEvent.type == Scenario::Event::Type::ShowMessage)
		{
			_pLabel->SetText("Display user message");
			CreateContentTextBox(storyEvent.content, "Type a message for the user", true);
		}

		if (_pTextBox)
			SetHeight(_pTextBox->GetBottom());
		else if (_pRoleDropList)
			SetHeight(_pRoleDropList->GetBottom());
		else
			SetHeight(23);
	}

	void StoryEventWidget::OnUpdate(float)
	{
		if (_pTextBox and _pTextBox->GetHeight() != _lastTextBoxHeight)
		{
			_lastTextBoxHeight = _pTextBox->GetHeight();
			SetHeight(_pTextBox->GetY() + _pTextBox->GetHeight());
			InvalidateParentLayout();
		}
	}

	void StoryEventWidget::OnSize()
	{
		if (_pTextBox)
			_pTextBox->SetWidth(GetWidth());

		if (_pSettingsButton)
			_pSettingsButton->SetX(std::min(GetWidth(), MaxWidth) - 20);
		if (_pMoveDownButton)
			_pMoveDownButton->SetX(std::min(GetWidth(), MaxWidth) - 42);
		if (_pMoveUpButton)
			_pMoveUpButton->SetX(std::min(GetWidth(), MaxWidth) - 64);
	}

	Role StoryEventWidget::GetRole() const noexcept
	{
		if (_pRoleDropList)
			return _pRoleDropList->GetValue().value_or(Role::Undefined);
		return Role::Undefined;
	}

	fig::string_view StoryEventWidget::GetContent() const noexcept
	{
		if (_pTextBox)
			return _pTextBox->GetText();
		else
		{
			static fig::string EmptyValue {};
			return EmptyValue;
		}
	}

	void StoryEventWidget::SetFocus(bool bFocus, TabDirection dir)
	{
		if (_pTextBox)
			return _pTextBox->SetFocus(bFocus, dir);
	}

	bool StoryEventWidget::IsFocused() const
	{
		if (_pTextBox)
			return _pTextBox->IsFocused();
		return false;
	}

	EventResult StoryEventWidget::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}

	void StoryEventWidget::OnMove(int32_t dir) noexcept
	{
		if (_fnMoveDelegate)
			_fnMoveDelegate(dir);
	}

	void StoryEventWidget::OnValueChanged() noexcept
	{
		if (_pMsgTypeDropList)
			_pMsgTypeDropList->SetEnabled(_pRoleDropList->GetValue() != Role::Narrator);
		if (_fnChangedDelegate)
			_fnChangedDelegate(GetValue());
	}
}