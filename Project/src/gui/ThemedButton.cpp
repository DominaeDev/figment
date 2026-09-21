#include <pch.h>
#include "gui/ThemedButton.h"
#include "gui/GUIConstants.h"

namespace fig::gui
{
	ThemedButton::ThemedButton(ControlPtr pParent) : Control(pParent), MouseEventHandler(this)
	{
		SetTheme(Theme::DefaultButtonStyle);
	}

	EventResult ThemedButton::OnEvent(fig::event& event)
	{
		return MouseEventHandler::HandleMouseEvents(event);
	}

	void ThemedButton::SetTheme(const ButtonTheme& theme) noexcept
	{
		_theme = theme;
		OnButtonState();
	}

	fig::color_ref ThemedButton::GetThemeForeground() const noexcept
	{
		switch (_state)
		{
		default:
			return _theme.defaultColor.foreground;
		case ButtonState::Hover:
			return _theme.hoverColor.foreground;
		case ButtonState::Pressed:
			return _theme.pressedColor.foreground;
		case ButtonState::Disabled:
			return _theme.disabledColor.foreground;
		}
	}

	fig::color_ref ThemedButton::GetThemeBackground() const noexcept
	{
		switch (_state)
		{
		default:
			return _theme.defaultColor.background;
		case ButtonState::Hover:
			return _theme.hoverColor.background;
		case ButtonState::Pressed:
			return _theme.pressedColor.background;
		case ButtonState::Disabled:
			return _theme.disabledColor.background;
		}
	}

	fig::color_ref ThemedButton::GetThemeBorderColor() const noexcept
	{
		switch (_state)
		{
		default:
			return _theme.defaultColor.border;
		case ButtonState::Hover:
			return _theme.hoverColor.border;
		case ButtonState::Pressed:
			return _theme.pressedColor.border;
		case ButtonState::Disabled:
			return _theme.disabledColor.border;
		}
	}

	void ThemedButton::OnAfterLayout()
	{
		DropState();
	}

	void ThemedButton::OnEnabled(bool bEnabled)
	{
		MouseEventHandler::Enable(bEnabled);
	}
}