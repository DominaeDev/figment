#pragma once

#include "gui/ColorRef.h"

namespace fig::gui
{
	struct ButtonTheme
	{
		color_set defaultColor;
		color_set hoverColor;
		color_set pressedColor;
		color_set disabledColor;
	};

	inline static ButtonTheme DefaultButtonStyle =
	{
		.defaultColor	{ Color::ButtonDefaultBackground, Color::ButtonDefaultForeground, Color::ButtonDefaultBorder },
		.hoverColor		{ Color::ButtonHoverBackground, Color::ButtonHoverForeground, Color::ButtonHoverBorder },
		.pressedColor	{ Color::ButtonPressedBackground, Color::ButtonPressedForeground, Color::ButtonPressedBorder },
		.disabledColor	{ Color::DisabledButtonBackground, Color::DisabledButtonForeground, Color::DisabledButtonBorder },
	};

	inline static ButtonTheme SidePanelButtonStyle =
	{
		.defaultColor	{ Color::SidePanelButtonDefaultBackground, Color::SidePanelButtonDefaultForeground, Color::Undefined },
		.hoverColor		{ Color::SidePanelButtonHoverBackground, Color::SidePanelButtonHoverForeground, Color::Undefined },
		.pressedColor	{ Color::SidePanelButtonPressedBackground, Color::SidePanelButtonPressedForeground, Color::Undefined },
		.disabledColor	{ Color::DisabledButtonBackground, Color::DisabledButtonForeground, Color::Undefined },
	};

	inline static ButtonTheme SaveButtonStyle =
	{
		.defaultColor	{ Color::SaveButtonDefaultBackground, Color::SaveButtonDefaultForeground, Color::SaveButtonDefaultBorder },
		.hoverColor		{ Color::SaveButtonHoverBackground, Color::SaveButtonHoverForeground, Color::SaveButtonHoverBorder },
		.pressedColor	{ Color::SaveButtonPressedBackground, Color::SaveButtonPressedForeground, Color::SaveButtonPressedBorder },
		.disabledColor	{ Color::DisabledButtonBackground, Color::DisabledButtonForeground, Color::DisabledButtonBorder },
	};

	inline static ButtonTheme PlayButtonStyle =
	{
		.defaultColor	{ Color::PlayButtonDefaultBackground, Color::PlayButtonDefaultForeground, Color::Undefined },
		.hoverColor		{ Color::PlayButtonHoverBackground, Color::PlayButtonHoverForeground, Color::Undefined },
		.pressedColor	{ Color::PlayButtonPressedBackground, Color::PlayButtonPressedForeground, Color::Undefined },
		.disabledColor	{ Color::DisabledButtonBackground, Color::DisabledButtonForeground, Color::Undefined },
	};
}