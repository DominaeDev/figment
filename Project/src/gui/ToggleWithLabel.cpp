#include <pch.h>
#include "gui/ToggleWithLabel.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorderRenderer.h"

namespace fig::gui
{
	ToggleWithLabel::ToggleWithLabel(ControlPtr pParent, fig::string_view text, double fontSize, ToggleBehavior behavior, bool bOn) : ButtonWithLabel(pParent, text, fontSize)
	{
		SetOn(bOn, true);

		switch (behavior)
		{
		default:
		case ToggleBehavior::Default:
			MouseEventHandler::SetDelegate([this]() { SetOn(!this->_bOn); });
			break;
		case ToggleBehavior::Radio:
			MouseEventHandler::SetDelegate([this]() { if (!this->_bOn) SetOn(true); });
			break;
		}
	}

	void ToggleWithLabel::SetDelegate(ToggleDelegate pDelegate) noexcept
	{
		_fnToggle = pDelegate;
	}

	void ToggleWithLabel::SetOn(bool bOn, bool bSilent) noexcept
	{
		_bOn = bOn;
		GetBorderRenderer()->SetColor(bOn ? Color::BorderSelected : Color::Border); 
		
		if (GetEnabled())
			SetButtonState(ButtonState::Default);

		if (_fnToggle and not bSilent)
			_fnToggle(_bOn);
	}

	void ToggleWithLabel::OnButtonState()
	{
		if (_state != ButtonState::Disabled && _bOn)
		{
			GetBackgroundRenderer()->SetColor(_theme.hoverColor.background);
			_pLabel->SetForegroundColor(_theme.hoverColor.foreground);
		}
		else
		{
			ButtonWithLabel::OnButtonState();
		}

	}
}