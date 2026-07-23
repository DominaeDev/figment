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
		inline constexpr fig::color Debug						{ 0xC000C0_rgb };
		inline constexpr fig::color Debug2						{ 0x00C0C0_rgb };
		inline constexpr fig::color Debug3						{ 0xC0C000_rgb };
		inline constexpr fig::color White						{ 0xFFFFFF_rgb };
		inline constexpr fig::color Black						{ 0x000000_rgb };
		inline constexpr fig::color Red							{ 0x800000_rgb };
		inline constexpr fig::color Green						{ 0x008000_rgb };
		inline constexpr fig::color Blue						{ 0x000080_rgb };
		inline constexpr fig::color DarkGray					{ 0x646464_rgb };
		inline constexpr fig::color Transparent					{ 0xFFFFFF00_rgba };
		inline constexpr fig::color TextForeground				{ 0x000000_rgb };
		inline constexpr fig::color TextSelectionForeground		{ 0xFFFFFF_rgb };
		inline constexpr fig::color TextSelectionBackground		{ 0x99C9EF_rgb };
		inline constexpr fig::color AppBackground				{ 0xfaf9f5_rgb };
		
		inline constexpr fig::color DisabledForeground			{ 0xa0a0a0_rgb };
		inline constexpr fig::color DisabledBackground			{ 0xe0e0e0_rgb };

		inline constexpr fig::color SidePanelForeground			{ 0x4e4431_rgb };
		inline constexpr fig::color SidePanelBackground			{ 0xEEECE4_rgb };
		inline constexpr fig::color SidePanelGradient			{ 0xe0dccb_rgb };
		inline constexpr fig::color LineColor					{ 0xc7bcaa_rgb };
		
		inline constexpr fig::color GenderTagMale				{ 0x45ccff_rgb };
		inline constexpr fig::color GenderTagFemale				{ 0xff5abb_rgb };
		inline constexpr fig::color GenderTagOther				{ 0x4cff8b_rgb };

		// Chat
		inline constexpr fig::color ChatBackground				{ 0xfaf9f5_rgb };
		inline constexpr fig::color MessageBorderDefault		{ 0x9f9f9f_rgb };
		inline constexpr fig::color MessageBackgroundDefault	{ 0xf4f4f4_rgb };
		inline constexpr fig::color MessageBorderBlue			{ 0x4da1c1_rgb };
		inline constexpr fig::color MessageBackgroundBlue		{ 0xf2fbff_rgb };
		inline constexpr fig::color MessageBorderPink			{ 0xef76bd_rgb };
		inline constexpr fig::color MessageBackgroundPink		{ 0xfff3f9_rgb };
		inline constexpr fig::color MessageBorderGreen			{ 0x50e433_rgb };
		inline constexpr fig::color MessageBackgroundGreen		{ 0xeaffe9_rgb };
		inline constexpr fig::color MessageBorderYellow			{ 0xe4c533_rgb };
		inline constexpr fig::color MessageBackgroundYellow		{ 0xfffcea_rgb };
		inline constexpr fig::color MessageBorderRed			{ 0xd52b2b_rgb };
		inline constexpr fig::color MessageBackgroundRed		{ 0xffeeee_rgb };
		inline constexpr fig::color MessageBorderTeal			{ 0x4dc1ba_rgb };
		inline constexpr fig::color MessageBackgroundTeal		{ 0xeefffc_rgb };
		inline constexpr fig::color MessageBorderPurple			{ 0xb25ce1_rgb };
		inline constexpr fig::color MessageBackgroundPurple		{ 0xfcf3ff_rgb };
		inline constexpr fig::color MessageBorderBrown			{ 0xc07c4c_rgb };
		inline constexpr fig::color MessageBackgroundBrown		{ 0xfff9ea_rgb };
		inline constexpr fig::color MessageBorderNavy			{ 0x4d55c1_rgb };
		inline constexpr fig::color MessageBackgroundNavy		{ 0xf2faff_rgb };

		inline constexpr fig::color DefaultUserMessageBorder = Colors::MessageBorderBlue;
		inline constexpr fig::color DefaultUserMessageBackground = Colors::MessageBackgroundBlue;

		inline constexpr std::array<fig::color, 8> DefaultBotMessageBorders {
			MessageBorderPink,
			MessageBorderGreen,
			MessageBorderYellow,
			MessageBorderRed,
			MessageBorderTeal,
			MessageBorderPurple,
			MessageBorderBrown,
			MessageBorderNavy,
		};

		inline constexpr std::array<fig::color, 8> DefaultBotMessageBackgrounds {
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
