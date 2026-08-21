#include <pch.h>
#include "gui/CheckBox.h"
#include "gui/CustomRenderers.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	CheckBox::CheckBox(ControlPtr pParent, fig::string_view label, bool bOn) : Control(pParent), MouseEventHandler(this),
		_bOn { bOn }
	{
		SetForegroundColor(0x4E4431_rgb);

		_pBox = CreateControl<Panel>();
		_pBox->SetSize(28, 28);

		auto pBoxBG = _pBox->SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pBoxBG->SetColor(Color::TextBoxBackground);
		auto pBoxBorder = _pBox->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pBoxBorder->SetColor(Color::LineColor);

		_pTick = _pBox->CreateControl<Image>(Resource::ICON_CHECKMARK);
		_pTick->SetForegroundColor(Color::Icon);
		_pTick->Center();
		_pTick->SetVisible(_bOn);

		_pLabel = CreateControl<StaticText>(label);
		_pLabel->EnableEllipsis(true);
		_pLabel->SetX(36);

		SetSize(300, 32);
	}

	void CheckBox::SetLabel(fig::string_view text) noexcept
	{
		_pLabel->SetText(text);
	}

	void CheckBox::SetValue(bool bOn, bool bSilent) noexcept
	{
		_bOn = bOn;

		_pTick->SetVisible(_bOn);

		if (_fnDelegate and not bSilent)
			_fnDelegate(_bOn);
	}

	void CheckBox::SetDelegate(OnCheckedDelegate fnDelegate)
	{
		_fnDelegate = fnDelegate;
	}

	EventResult CheckBox::OnEvent(fig::event& event)
	{
		if (auto result = MouseEventHandler::HandleMouseEvents(event); result == EventResult::Handled)
			return result;

		return Control::OnEvent(event);
	}

	void CheckBox::OnSize()
	{
		_pBox->CenterVertically();
		_pLabel->SetWidth(GetWidth() - _pLabel->GetX());
		_pLabel->CenterVertically();
	}

	void CheckBox::OnEnabled(bool bEnabled)
	{
		MouseEventHandler::Enable(bEnabled);
	}

	void CheckBox::OnClicked()
	{
		SetValue(not GetValue());
	}

	void CheckBox::OnButtonState()
	{
		switch (_state)
		{
		case ButtonState::Default:
		case ButtonState::Hover:
			_pBox->GetBackgroundRenderer()->SetColor(Color::TextBoxBackground);
			_pBox->GetBorderRenderer()->SetColor(Color::LineColor);
			_pLabel->SetForegroundColor(0x4E4431_rgb);
			_pTick->SetForegroundColor(Color::Icon);
			break;
		case ButtonState::Pressed:
			_pBox->GetBackgroundRenderer()->SetColor(0xF0F0F0_rgb);
			_pBox->GetBorderRenderer()->SetColor(Color::LineColor);
			_pLabel->SetForegroundColor(0x4E4431_rgb);
			_pTick->SetForegroundColor(Color::Icon);
			break;
		case ButtonState::Disabled:
			_pBox->GetBackgroundRenderer()->SetColor(Color::DisabledBackground);
			_pBox->GetBorderRenderer()->SetColor(Color::DisabledLineColor);
			_pLabel->SetForegroundColor(0x808080C0_rgba);
			_pTick->SetForegroundColor(Color::DisabledForeground);
			break;
		}
	}

}