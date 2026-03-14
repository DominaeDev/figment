#ifndef GUI_CONSTANTS_H__
#define GUI_CONSTANTS_H__
#pragma once

#include "GUITypes.h"

namespace fig::gui
{
	namespace Themes
	{
		constexpr ButtonTheme DefaultButtonStyle
		{
			.defaultColor	{ Color { 0x4e4431 }, Color { 0xFFFFFF, 0x00 } },
			.hoverColor		{ Color { 0x4e4431 }, Color { 0xFFFFFF, 0x80 } },
			.pressedColor	{ Color { 0x4e4431 }, Color { 0xFFFFFF, 0xC0 } },
			.disabledColor	{ Color { 0x808080 }, Color { 0xCCCCCC, 0x80 } },
		};

		constexpr ButtonTheme SidePanelButtonStyle
		{
			.defaultColor	{ Color { 0x4e4431 }, Color { 0xFFFFFF, 0x00 } },
			.hoverColor		{ Color { 0x4e4431 }, Color { 0xe4e0d1, 0xff } },
			.pressedColor	{ Color { 0x4e4431 }, Color { 0xdfd5c3, 0xff } },
			.disabledColor	{ Color { 0x4e4431 }, Color { 0xe1dfd8, 0xff } },
		};
	}

	namespace Colors
	{
		inline constexpr Color White					{ 0xFFFFFF };
		inline constexpr Color Black					{ 0x000000 };
		inline constexpr Color Red						{ 0x800000 };
		inline constexpr Color Green					{ 0x008000 };
		inline constexpr Color Blue						{ 0x000080 };
		inline constexpr Color DarkGray					{ 0x646464 };
		inline constexpr Color Transparent				{ 0xFFFFFF, 0x00 };
		inline constexpr Color Debug					{ 0xC000C0 };
		inline constexpr Color TextForeground			{ 0x000000 };
		inline constexpr Color TextSelectionForeground	{ 0xFFFFFF };
		inline constexpr Color TextSelectionBackground	{ 0x99C9EF };
		inline constexpr Color AppBackground			{ 0xfaf9f5 };

		inline constexpr Color SidePanelForeground		{ 0x4e4431 };
		inline constexpr Color SidePanelBackground		{ 0xEEECE4 };
		inline constexpr Color SidePanelGradient		{ 0xe0dccb };
		
		// Chat
		inline constexpr Color ChatBackground			{ 0xfaf9f5, 0xff };
		inline constexpr Color MessageBorderDefault		{ 0x9f9f9f };
		inline constexpr Color MessageBackgroundDefault	{ 0xf4f4f4 };
		inline constexpr Color MessageBorderBlue		{ 0x4da1c1 };
		inline constexpr Color MessageBackgroundBlue	{ 0xf2fbff };
		inline constexpr Color MessageBorderPink		{ 0xef76bd };
		inline constexpr Color MessageBackgroundPink	{ 0xfff3f9 };
		inline constexpr Color MessageBorderGreen		{ 0x50e433 };
		inline constexpr Color MessageBackgroundGreen	{ 0xeaffe9 };
		inline constexpr Color MessageBorderYellow		{ 0xe4c533 };
		inline constexpr Color MessageBackgroundYellow	{ 0xfffcea };
		inline constexpr Color MessageBorderRed			{ 0xd52b2b };
		inline constexpr Color MessageBackgroundRed		{ 0xffeeee };
		inline constexpr Color MessageBorderTeal		{ 0x4dc1ba };
		inline constexpr Color MessageBackgroundTeal	{ 0xeefffc };
		inline constexpr Color MessageBorderPurple		{ 0xb25ce1 };
		inline constexpr Color MessageBackgroundPurple	{ 0xfcf3ff };
		inline constexpr Color MessageBorderBrown		{ 0xc07c4c };
		inline constexpr Color MessageBackgroundBrown	{ 0xfff9ea };
		inline constexpr Color MessageBorderNavy		{ 0x4d55c1 };
		inline constexpr Color MessageBackgroundNavy	{ 0xf2faff };

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

#endif