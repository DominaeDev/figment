#pragma once

#include "GUITypes.h"

namespace fig::gui
{
	namespace Themes
	{
		constexpr ButtonTheme DefaultButtonStyle
		{
			.defaultColor	{ 0x4e4431_rgb, 0xFFFFFF00_rgba },
			.hoverColor		{ 0x4e4431_rgb, 0xefece3FF_rgba },
			.pressedColor	{ 0x4e4431_rgb, 0xFFFFFFC0_rgba },
			.disabledColor	{ 0x4e4431_rgb, 0xCCCCCC80_rgba },
		};

		constexpr ButtonTheme SidePanelButtonStyle
		{
			.defaultColor	{ 0x4e4431_rgb, 0xFFFFFF00_rgba },
			.hoverColor		{ 0x4e4431_rgb, 0xFFFFFF80_rgba },
			.pressedColor	{ 0x4e4431_rgb, 0xFFFFFFC0_rgba },
			.disabledColor	{ 0x808080_rgb, 0xCCCCCC80_rgba },
		};
	}

	namespace Colors
	{
		inline constexpr Color White					{ 0xFFFFFF_rgb };
		inline constexpr Color Black					{ 0x000000_rgb };
		inline constexpr Color Red						{ 0x800000_rgb };
		inline constexpr Color Green					{ 0x008000_rgb };
		inline constexpr Color Blue						{ 0x000080_rgb };
		inline constexpr Color DarkGray					{ 0x646464_rgb };
		inline constexpr Color Transparent				{ 0xFFFFFF00_rgba };
		inline constexpr Color Debug					{ 0xC000C0_rgb };
		inline constexpr Color TextForeground			{ 0x000000_rgb };
		inline constexpr Color TextSelectionForeground	{ 0xFFFFFF_rgb };
		inline constexpr Color TextSelectionBackground	{ 0x99C9EF_rgb };
		inline constexpr Color AppBackground			{ 0xfaf9f5_rgb };
		
		inline constexpr Color DisabledForeground		{ 0xa0a0a0_rgb };
		inline constexpr Color DisabledBackground		{ 0xe0e0e0_rgb };

		inline constexpr Color SidePanelForeground		{ 0x4e4431_rgb };
		inline constexpr Color SidePanelBackground		{ 0xEEECE4_rgb };
		inline constexpr Color SidePanelGradient		{ 0xe0dccb_rgb };
		inline constexpr Color LineColor				{ 0xc7bcaa_rgb };
		
		inline constexpr Color GenderTagMale			{ 0x45ccff_rgb };
		inline constexpr Color GenderTagFemale			{ 0xff5abb_rgb };
		inline constexpr Color GenderTagOther			{ 0x4cff8b_rgb };

		// Chat
		inline constexpr Color ChatBackground			{ 0xfaf9f5_rgb };
		inline constexpr Color MessageBorderDefault		{ 0x9f9f9f_rgb };
		inline constexpr Color MessageBackgroundDefault	{ 0xf4f4f4_rgb };
		inline constexpr Color MessageBorderBlue		{ 0x4da1c1_rgb };
		inline constexpr Color MessageBackgroundBlue	{ 0xf2fbff_rgb };
		inline constexpr Color MessageBorderPink		{ 0xef76bd_rgb };
		inline constexpr Color MessageBackgroundPink	{ 0xfff3f9_rgb };
		inline constexpr Color MessageBorderGreen		{ 0x50e433_rgb };
		inline constexpr Color MessageBackgroundGreen	{ 0xeaffe9_rgb };
		inline constexpr Color MessageBorderYellow		{ 0xe4c533_rgb };
		inline constexpr Color MessageBackgroundYellow	{ 0xfffcea_rgb };
		inline constexpr Color MessageBorderRed			{ 0xd52b2b_rgb };
		inline constexpr Color MessageBackgroundRed		{ 0xffeeee_rgb };
		inline constexpr Color MessageBorderTeal		{ 0x4dc1ba_rgb };
		inline constexpr Color MessageBackgroundTeal	{ 0xeefffc_rgb };
		inline constexpr Color MessageBorderPurple		{ 0xb25ce1_rgb };
		inline constexpr Color MessageBackgroundPurple	{ 0xfcf3ff_rgb };
		inline constexpr Color MessageBorderBrown		{ 0xc07c4c_rgb };
		inline constexpr Color MessageBackgroundBrown	{ 0xfff9ea_rgb };
		inline constexpr Color MessageBorderNavy		{ 0x4d55c1_rgb };
		inline constexpr Color MessageBackgroundNavy	{ 0xf2faff_rgb };

		inline constexpr Color DefaultUserMessageBorder = Colors::MessageBorderBlue;
		inline constexpr Color DefaultUserMessageBackground = Colors::MessageBackgroundBlue;

		inline constexpr std::array<Color, 8> DefaultBotMessageBorders {
			MessageBorderPink,
			MessageBorderGreen,
			MessageBorderYellow,
			MessageBorderRed,
			MessageBorderTeal,
			MessageBorderPurple,
			MessageBorderBrown,
			MessageBorderNavy,
		};

		inline constexpr std::array<Color, 8> DefaultBotMessageBackgrounds {
			MessageBackgroundPink,
			MessageBackgroundGreen,
			MessageBackgroundYellow,
			MessageBackgroundRed,
			MessageBackgroundTeal,
			MessageBackgroundPurple,
			MessageBackgroundBrown,
			MessageBackgroundNavy,
		};
	}
}
