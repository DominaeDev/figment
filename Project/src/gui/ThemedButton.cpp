#include <pch.h>
#include "gui/ThemedButton.h"
#include "gui/GUIConstants.h"

namespace fig::gui
{
	ThemedButton::ThemedButton(ControlPtr pParent) : Control(pParent), BaseButton(this)
	{
		SetTheme(Theme::DefaultButtonStyle);
	}

	EventResult ThemedButton::OnEvent(fig::event& event)
	{
		return HandleMouseEvents(event) ? EventResult::Handled : EventResult::Pass;
	}

	void ThemedButton::SetTheme(const ButtonTheme& theme) noexcept
	{
		_theme = theme;
	}

	const fig::color& ThemedButton::GetThemeForeground() const noexcept
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

	const fig::color& ThemedButton::GetThemeBackground() const noexcept
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

	void ThemedButton::OnAfterLayout()
	{
		DropState();
	}
}