#include <pch.h>
#include "gui/StoryEventWidget.h"
#include "gui/TextBox.h"
#include "gui/TextInput.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithIcon.h"
#include "gui/VerticalBar.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"
#include "gui/DefaultTextStyleProvider.h"

using namespace fig::chat;
using namespace fig::data;

namespace fig::gui
{
	constexpr fig::coord MaxWidth = 620;
	constexpr fig::coord Spacing = 6;
	constexpr fig::coord LeftMargin = 12;

	StoryEventWidget::StoryEventWidget(ControlPtr pParent, const Scenario::Event& storyEvent, fig::observer_ptr<const Scenario> pScenario) : Control(pParent),
		_pScenario { pScenario },
		_eventType { storyEvent.type },
		_role { storyEvent.role }
	{
		_pLabel = CreateControl<StaticText>("", FontFace::Default, 14.0, true);
		_pLabel->SetPosition(25 + LeftMargin, 1);
		_pLabel->SetForegroundColor(Color::SidePanelForeground);

		_pIcon = CreateControl<Image>(Resource::NONE, Color::Icon);
		_pIcon->SetX(LeftMargin);

		_pSettingsButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_SETTINGS);
		_pSettingsButton->SetSize(20, 20);
		_pMoveUpButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_MOVE_UP);
		_pMoveUpButton->SetSize(20, 20);
		_pMoveUpButton->SetDelegate([this] { OnMove(-1); });
		_pMoveDownButton = CreateControl<ButtonWithIcon>(Resource::ICON_CHARACTER_EDIT_ATTRIBUTE_MOVE_DOWN);
		_pMoveDownButton->SetSize(20, 20);
		_pMoveDownButton->SetDelegate([this] { OnMove(1); });

		_pColorBar = CreateControl<VerticalBar>(Resource::VERTICAL_BAR_SMALL);
		_pColorBar->SetWidth(4);
		_pColorBar->SetForegroundColor(Color::Accent);

		InitValue(storyEvent.type, storyEvent.role, storyEvent.content);
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
		case Scenario::Event::Type::ChatDialogue:
		case Scenario::Event::Type::ChatAction:
		case Scenario::Event::Type::ChatNarration:
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
		pDropList->SetPosition(LeftMargin, _controlY);

		for (auto& role : _pScenario->roles)
			pDropList->AddItem(role.label, role.role);
		pDropList->SelectValue(role);

		_controlY += pDropList->GetHeight() + Spacing;
		_pDropList.reset(pDropList);
	}

	void StoryEventWidget::CreateMessageTypeDropList(MessageType msgType)
	{
		auto _pMsgTypeDropList = CreateControl<DropListOfType<MessageType>>();
		_pMsgTypeDropList->SetWidth(240);
		_pMsgTypeDropList->SetDelegate([this](auto&& value) { OnValueChanged(); });
		_pMsgTypeDropList->SetPosition(248, 24);

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
		_pTextBox->SetPosition(LeftMargin, _controlY);
		_pTextBox->EnableAutoSize(true);
		_pTextBox->SetMode(bMultiline ? TextInput::Mode::Multiline : TextInput::Mode::SingleWordWrap);
		_pTextBox->SetMinRows(bMultiline ? 2 : 1);
		_pTextBox->SetMaxRows(bMultiline ? 12 : 4);
		_pTextBox->SetMaxWidth(MaxWidth - LeftMargin);
		_pTextBox->SetWidth(MaxWidth - LeftMargin);
		_pTextBox->SetText(text);
		_pTextBox->SetPlaceholder(placeholder);
	}

	void StoryEventWidget::InitValue(Scenario::Event::Type eventType, Role role, fig::string_view content)
	{
		_controlY = 24;
		
		if (eventType == Scenario::Event::Type::ChatDialogue
			or eventType == Scenario::Event::Type::ChatAction
			or eventType == Scenario::Event::Type::ChatNarration)
		{
			if (not (is_bot(_role) or is_user(_role)))
				_role = Role::Bot1;
		}
		else if (eventType == Scenario::Event::Type::ChatNarration)
			_role = Role::Narrator;

		DestroyChild(_pDropList);
		_pDropList.reset();
		DestroyChild(_pTextBox);
		_pTextBox.reset();

		if (eventType == Scenario::Event::Type::ChatDialogue)
		{
			_pLabel->SetText("Dialogue from");
			_pIcon->SetTexture(Resource::ICON_STORY_EDIT_DIALOGUE, true);
			CreateRoleDropList(role);
			CreateContentTextBox(content, "Nice to meet you", true);
		}
		else if (eventType == Scenario::Event::Type::ChatAction)
		{
			_pLabel->SetText("Action from");
			_pIcon->SetTexture(Resource::ICON_STORY_EDIT_ACTION, true);
			CreateRoleDropList(role);
			CreateContentTextBox(content, "smiles warmly", true);
		}
		else if (eventType == Scenario::Event::Type::ChatNarration)
		{
			_pLabel->SetText("Narration");
			_pIcon->SetTexture(Resource::ICON_STORY_EDIT_NARRATION, true);
			CreateContentTextBox(content, "Once upon a time...", true);
		}
		else if (eventType == Scenario::Event::Type::Instigate)
		{
			_pLabel->SetText("Instigate response from");
			_pIcon->SetTexture(Resource::ICON_STORY_EDIT_INSTIGATE, true);
			CreateRoleDropList(role);
		}
		else if (eventType == Scenario::Event::Type::Instruct)
		{
			_pLabel->SetText("Instruction");
			_pIcon->SetTexture(Resource::ICON_STORY_EDIT_INSTRUCT, true);
			CreateContentTextBox(content, "Type an instruction for the AI to follow", false);
		}
		else if (eventType == Scenario::Event::Type::ShowMessage)
		{
			_pLabel->SetText("Display user message");
			_pIcon->SetTexture(Resource::ICON_STORY_EDIT_USER_MESSAGE, true);
			CreateContentTextBox(content, "Type a message for the user", true);
		}
		_pColorBar->SetHeight(0);

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
			_pTextBox->SetWidth(GetWidth() - LeftMargin);

		if (_pSettingsButton)
			_pSettingsButton->SetX(std::min(GetWidth(), MaxWidth) - 20);
		if (_pMoveDownButton)
			_pMoveDownButton->SetX(std::min(GetWidth(), MaxWidth) - 42);
		if (_pMoveUpButton)
			_pMoveUpButton->SetX(std::min(GetWidth(), MaxWidth) - 64);

		_pColorBar->SetY(3);
		_pColorBar->SetHeight(GetHeight() - 3);
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

	void StoryEventWidget::ChangeEventType(Scenario::Event::Type eventType)
	{
		_eventType = eventType;
		InitValue(eventType, _role, fig::string { GetContent() });
		InvalidateParentLayout();
	}
}