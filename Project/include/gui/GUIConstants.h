#pragma once

#include "GUITypes.h"

namespace fig::gui
{
	namespace Theme
	{
		inline static ButtonTheme DefaultButtonStyle = 
		{
			.defaultColor	{ custom_color(0xFFFFFF00_rgba), custom_color(0x4E4431_rgb), Color::LineColor },
			.hoverColor		{ custom_color(0xEFECE3FF_rgba), custom_color(0x4E4431_rgb), Color::LineColor },
			.pressedColor	{ custom_color(0xFFFFFFC0_rgba), custom_color(0x4E4431_rgb), Color::LineColor },
			.disabledColor	{ custom_color(0xCCCCCC80_rgba), custom_color(0x808080_rgb), Color::DisabledLineColor },
		};

		inline static ButtonTheme SidePanelButtonStyle =
		{
			.defaultColor	{ custom_color(0xFFFFFF00_rgba), custom_color(0x4E4431_rgb), Color::LineColor },
			.hoverColor		{ custom_color(0xFFFFFF80_rgba), custom_color(0x4E4431_rgb), Color::LineColor },
			.pressedColor	{ custom_color(0xFFFFFFC0_rgba), custom_color(0x4E4431_rgb), Color::LineColor },
			.disabledColor	{ custom_color(0xCCCCCC80_rgba), custom_color(0x808080_rgb), Color::DisabledLineColor },
		};

		inline static ButtonTheme EditorSmallButtonStyle =
		{
			.defaultColor	{ custom_color(0xFFFFFF00_rgba), custom_color(0x8b806b_rgb), Color::LineColor },
			.hoverColor		{ custom_color(0xEFECE3FF_rgba), custom_color(0x8b806b_rgb), Color::LineColor },
			.pressedColor	{ custom_color(0xFFFFFFC0_rgba), custom_color(0x8b806b_rgb), Color::LineColor },
			.disabledColor	{ custom_color(0xCCCCCC80_rgba), custom_color(0x808080_rgb), Color::DisabledLineColor },
		};

		inline static ButtonTheme GreenSaveButtonStyle =
		{
			.defaultColor	{ custom_color(0xc7e8c1_rgb),		custom_color(0x13330e_rgb), custom_color(0x097f00_rgb) },
			.hoverColor		{ custom_color(0x77ce70C0_rgba),	custom_color(0x13330e_rgb), custom_color(0x097f00_rgb) },
			.pressedColor	{ custom_color(0xb7e3b1_rgb),		custom_color(0x13330e_rgb), custom_color(0x097f00_rgb) },
			.disabledColor	{ custom_color(0xCCCCCC80_rgba),	custom_color(0x808080_rgb), Color::DisabledLineColor },
		};
	}
}
