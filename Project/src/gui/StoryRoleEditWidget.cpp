#include <pch.h>
#include "gui/StoryRoleEditWidget.h"
#include "gui/TextBox.h"
#include "gui/TextInput.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithIcon.h"
#include "gui/VerticalBar.h"
#include "gui/CheckBox.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"
#include "gui/DefaultTextStyleProvider.h"
#include "data/Character.h"
#include "text/Condition.h"

using namespace fig::chat;
using namespace fig::data;

namespace fig::gui
{
	constexpr fig::coord MaxWidth = 620;
	constexpr fig::coord Spacing = 6;
	constexpr fig::coord LeftMargin = 12;
	constexpr fig::coord LabelMargin = LeftMargin + 4;

	StoryRoleEditWidget::StoryRoleEditWidget(ControlPtr pParent, fig::observer_ptr<Scenario> pScenario, fig::observer_ptr<const fig::data::Character> pCharacter, fig::chat::Role role) : Control(pParent),
		_pScenario { pScenario },
		_pCharacter { pCharacter },
		_role { role }
	{
		auto pSizer = SetSizer<VerticalSizer>();

		auto pHorizontalSizer = new HorizontalSizer();
		pSizer->Add(pHorizontalSizer, 0, SizerFlag::FixedSize, 56);

		auto pLeftSizer = new VerticalSizer();
		auto pRightSizer = new VerticalSizer();
		pHorizontalSizer->Add(pLeftSizer, 0, SizerFlag::FixedSize, 280);
		pHorizontalSizer->Add(pRightSizer, -1);

		// Label
		CreateLabel(pLeftSizer, "Name");
		_pName = CreateTextBox(pLeftSizer, "", "", TextInput::Mode::Single);
		_pName->SetTextChangedDelegate([this](auto&& value) { OnChangeName(value); });
		_pName->SetMaxWidth(260);

		// Identifier
		CreateLabel(pRightSizer, "Custom identifier");
		_pHandle = CreateTextBox(pRightSizer, "", "", TextInput::Mode::Single);
		_pHandle->SetTextChangedDelegate([this](auto&& value) { OnChangeHandle(value); });
		_pHandle->SetMaxWidth(260);
		pSizer->AddSpacer(Spacing);

		// Brief
		CreateLabel(pSizer, "Brief");
		_pBrief = CreateTextBox(pSizer, "", "", TextInput::Mode::SingleWordWrap);
		_pBrief->SetTextChangedDelegate([this](auto&& value) { OnChangeBrief(value); });
		_pBrief->SetPlaceholder("A brief description of this role, in relation to the user and other roles");
//		CreateHint(pSizer, "Briefly, describe this role's purpose and relationships to other roles.");
		
		pSizer->AddSpacer(Spacing);

		// Prompt
		CreateLabel(pSizer, "Description");
		_pPrompt = CreateTextBox(pSizer, "", "", TextInput::Mode::Multiline);
		_pPrompt->SetTextChangedDelegate([this](auto&& value) { OnChangePrompt(value); });
		_pPrompt->SetPlaceholder("Additional information that is specific to this role");
		
//		CreateHint(pSizer, "Details that are unique to this role go here.");

		pSizer->AddSpacer(Spacing);

		// Prompt
		CreateLabel(pSizer, "Character requirements");
		_pCondition = CreateTextBox(pSizer, "", "E.g., male or female", TextInput::Mode::Single);
		_pCondition->ClearStyle();
		_pCondition->SetTextChangedDelegate([this](auto&& value) { OnChangeCondition(value); });

		_pColorBar = CreateControl<VerticalBar>(Resource::VERTICAL_BAR_SMALL);
		_pColorBar->SetWidth(4);
		_pColorBar->SetForegroundColor(Color::Border);

		CreateTabGroup();

		InitValue();
	}

	fig::observer_ptr<fig::gui::StaticText> StoryRoleEditWidget::CreateHint(SizerPtr pSizer, fig::string_view text)
	{
		auto pHint = CreateControl<StaticText>(text, FontFace::Italic, 14.0, false);
		pHint->SetForegroundColor(Color::HintText);
		pSizer->Add(pHint, 0, SizerFlag::Expand | SizerFlag::Left, LabelMargin);
		return pHint;
	}

	fig::observer_ptr<StaticText> StoryRoleEditWidget::CreateLabel(SizerPtr pSizer, fig::string_view text)
	{
		auto pLabel = CreateControl<StaticText>(text, FontFace::Default, 14.0, true);
		pLabel->SetForegroundColor(Color::SidePanelForeground);
		pSizer->Add(pLabel, 0, SizerFlag::Expand | SizerFlag::Left, LabelMargin);
		return pLabel;
	}

	fig::observer_ptr<TextBox> StoryRoleEditWidget::CreateTextBox(SizerPtr pSizer, fig::string_view text, fig::string_view placeholder, TextInput::Mode mode)
	{
		bool bMultiline = mode == TextInput::Mode::Multiline;
		auto pTextBox = CreateControl<TextBox>(FontFace::Default, 14.0);
		pTextBox->SetStyleProvider<DefaultTextStyleProvider>();
		pTextBox->EnableAutoSize(true);
		pTextBox->SetMode(mode);
		pTextBox->SetMinRows(bMultiline ? 2 : 1);
		pTextBox->SetMaxRows(bMultiline ? 12 : 4);
		pTextBox->SetMaxWidth(MaxWidth - LeftMargin);
		pTextBox->SetWidth(MaxWidth - LeftMargin);
		pTextBox->SetText(text);
		pTextBox->SetPlaceholder(placeholder);
		pSizer->Add(pTextBox, 0, SizerFlag::Expand | SizerFlag::Left, LeftMargin);
		return pTextBox;
	}

	void StoryRoleEditWidget::Refresh()
	{
		InitValue();
	}

	void StoryRoleEditWidget::InitValue()
	{
		if (auto roleSlot = _pScenario->GetRoleSlot(_role))
		{
			_pName->SetText((*roleSlot).label);
			_pHandle->SetText((fig::string)(*roleSlot).customId);
			_pBrief->SetText((*roleSlot).brief);
			_pPrompt->SetText((*roleSlot).prompt);
		}
		RefreshHeight();
	}

	bool StoryRoleEditWidget::RefreshHeight()
	{
		if (auto height = _pBrief->GetHeight() + _pPrompt->GetHeight(); height != _lastTextBoxHeight)
		{
			_lastTextBoxHeight = height;

			_pColorBar->SetHeight(0);
			LayoutNow();
			ResizeToFit(false, true);

			_pColorBar->SetHeight(GetHeight() - 3);
			InvalidateParentLayout();
			return true;
		}
		return false;
	}

	void StoryRoleEditWidget::OnUpdate(float)
	{
		RefreshHeight();
	}

	void StoryRoleEditWidget::OnSize()
	{
		_pColorBar->SetY(3);
		_pColorBar->SetHeight(GetHeight() - 3);
	}

	void StoryRoleEditWidget::SetFocus(bool bFocus, TabDirection dir)
	{
		if (bFocus)
		{
			if (dir == TabDirectionForward)
				FocusNext(_pName, _pHandle, _pBrief, _pPrompt, _pCondition);
			else if (dir == TabDirectionBackward)
				FocusPrev(_pName, _pHandle, _pBrief, _pPrompt, _pCondition);
		}
		else
		{
			_pName->SetFocus(false);
			_pHandle->SetFocus(false);
			_pBrief->SetFocus(false);
			_pPrompt->SetFocus(false);
			_pCondition->SetFocus(false);
		}
	}

	bool StoryRoleEditWidget::IsFocused() const
	{
		return (bool)FindFocused(_pName, _pHandle, _pBrief, _pPrompt, _pCondition);
	}

	bool StoryRoleEditWidget::CanFocus() const
	{
		return GetEnabled() and GetVisible();
	}

	void StoryRoleEditWidget::OnChangeName(fig::string_view value)
	{
		if (auto roleSlot = _pScenario->GetRoleSlot(_role))
			(*roleSlot).label = fig::string { value };

		if (_fnNameChanged)
			_fnNameChanged(value);
	}

	void StoryRoleEditWidget::OnChangeHandle(fig::string_view value)
	{
		if (auto roleSlot = _pScenario->GetRoleSlot(_role))
			(*roleSlot).customId = fig::handle { value };
	}

	void StoryRoleEditWidget::OnChangeBrief(fig::string_view value)
	{
		if (auto roleSlot = _pScenario->GetRoleSlot(_role))
			(*roleSlot).brief = fig::string { value };
	}

	void StoryRoleEditWidget::OnChangePrompt(fig::string_view value)
	{
		if (auto roleSlot = _pScenario->GetRoleSlot(_role))
			(*roleSlot).prompt = fig::string { value };
	}

	void StoryRoleEditWidget::OnChangeCondition(fig::string_view value)
	{
		if (auto roleSlot = _pScenario->GetRoleSlot(_role))
		{
			(*roleSlot).validation.condition = Condition(fig::string { value }, true);
			_pCondition->SetForegroundColor((*roleSlot).validation.condition.IsOk() ? Color::TextBoxForeground : Color::ErrorText);
			_pCondition->InvalidateText();

		}
	}

	void StoryRoleEditWidget::SetLocked(bool bLocked)
	{
		_pName->SetEnabled(not bLocked);
		_pHandle->SetEnabled(not bLocked);
		_pCondition->SetEnabled(not bLocked);

		if (bLocked)
		{
			if (is_bot(_role))
			{
				_pHandle->SetText("character");

				if (_pCharacter)
					_pName->SetText(_pCharacter->name.GetSpokenName());
			}
			if (is_user(_role))
				_pHandle->SetText("user");
		}
	}
}