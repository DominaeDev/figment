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
	constexpr fig::coord Margin = 6;

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

	Scenario::Event StoryEventWidget::GetValue() const noexcept
	{
		auto value = Scenario::Event {
			.type = _eventType,
			.content = fig::string { GetContent() },
		};

		switch (_eventType)
		{
		case Scenario::Event::Type::ChatMessage:
			value.role = _role;
			value.msgType = _msgType;
			break;
		case Scenario::Event::Type::Instigate:
			value.role = _role;
			break;
		}

		return value;
	}

	void StoryEventWidget::SetValueChangedDelegate(ValueChangedDelegate fnDelegate)
	{
		_fnChangedDelegate = fnDelegate;
	}

	void StoryEventWidget::CreateRoleDropList(Role role)
	{
		auto pDropList = CreateControl<DropListOfType<Role>>();
		pDropList->SetWidth(240);
		pDropList->SetValueDelegate([this](auto&& value) { 
			_role = value;
			_msgType = {};
			OnValueChanged();
		});
		pDropList->SetPosition(0, _controlY);

		for (auto& role : _pScenario->roles)
			pDropList->AddItem(role.label, role.role);
		pDropList->AddItem("Narrator", Role::Narrator);
		pDropList->SelectValue(role);

		_controlY += pDropList->GetHeight() + Margin;

	}

	void StoryEventWidget::CreateRoleDropList(Role role, MessageType msgType)
	{
		auto pDropList = CreateControl<DropListOfType<std::pair<fig::chat::Role, fig::chat::MessageType>>>();
		pDropList->SetWidth(300);
		pDropList->SetValueDelegate([this](auto&& value) {
			_role = value.first;
			_msgType = value.second;
			OnValueChanged();
		});
		pDropList->SetPosition(0, _controlY);

		if (role == Role::Undefined)
			role = Role::Bot1;
		if (msgType == MessageType::Undefined)
			msgType = MessageType::Dialogue;

		for (auto& role : _pScenario->roles)
		{
			pDropList->AddItem(std::format("{} (Dialogue)", role.label), { role.role, MessageType::Dialogue });
			pDropList->AddItem(std::format("{} (Action)", role.label), { role.role, MessageType::Action });
			if constexpr (Disabled)
			{
				pDropList->AddItem(std::format("{} (Thought):", role.label), { role.role, MessageType::Thought });
			}
		}
		pDropList->AddItem("Narration", { Role::Narrator, MessageType::Narration });
		pDropList->SelectValue({ role, msgType });

		_controlY += pDropList->GetHeight() + Margin;
	}

	void StoryEventWidget::CreateMessageTypeDropList(MessageType msgType)
	{
		auto _pMsgTypeDropList = CreateControl<DropListOfType<MessageType>>();
		_pMsgTypeDropList->SetWidth(240);
		_pMsgTypeDropList->SetDelegate([this](auto&& value) { OnValueChanged(); });
		_pMsgTypeDropList->SetPosition(248, 23);

		if (msgType == MessageType::Undefined)
			msgType = MessageType::Dialogue;

		_pMsgTypeDropList->AddItem("Dialogue", MessageType::Dialogue);
		_pMsgTypeDropList->AddItem("Action", MessageType::Action);
		_pMsgTypeDropList->AddItem("Internal thought", MessageType::Thought);
		_pMsgTypeDropList->SelectValue(msgType);

	}

	void StoryEventWidget::CreateContentTextBox(fig::string_view text, fig::string_view placeholder, bool bMultiline)
	{
		_pTextBox = CreateControl<TextBox>(FontFace::Default, 14.0);
		_pTextBox->SetStyleProvider<DefaultTextStyleProvider>();
		_pTextBox->SetTextChangedDelegate([this](auto&& value) { OnValueChanged(); });
		_pTextBox->SetPosition(0, _controlY);
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
		_controlY = 23;

		if (storyEvent.type == Scenario::Event::Type::ChatMessage)
		{
			_pLabel->SetText("Add chat message");

			CreateRoleDropList(storyEvent.role, storyEvent.msgType);
			CreateContentTextBox(storyEvent.content, "Type a chat message", true);
		}
		else if (storyEvent.type == Scenario::Event::Type::Instigate)
		{
			_pLabel->SetText("Instigate response from");
			CreateRoleDropList(storyEvent.role);
		}
		else if (storyEvent.type == Scenario::Event::Type::Instruct)
		{
			_pLabel->SetText("Instruction");
			CreateContentTextBox(storyEvent.content, "Type an instruction for the AI to follow", false);
		}
		else if (storyEvent.type == Scenario::Event::Type::ShowMessage)
		{
			_pLabel->SetText("Display user message");
			CreateContentTextBox(storyEvent.content, "Type a message for the user", true);
		}

		ResizeToFit(false, true);
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

	bool StoryEventWidget::CanFocus() const
	{
		return _pTextBox;
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
		if (_fnChangedDelegate)
			_fnChangedDelegate(GetValue());
	}
}