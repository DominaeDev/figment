#pragma once

#include <SDL3/SDL.h>
#include "Types.h"

namespace fig::gui
{
	using Colorf = SDL_FColor;
	using Color = SDL_Color;

	namespace Colors
	{
		inline constexpr Color White { 0xFF, 0xFF, 0xFF, 0xFF };
		inline constexpr Color Black { 0x00, 0x00, 0x00, 0xFF };
		inline constexpr Color Red { 0x80, 0x00, 0x00, 0xFF };
		inline constexpr Color Green { 0x00, 0x80, 0x00, 0xFF };
		inline constexpr Color Blue { 0x00, 0x00, 0x80, 0xFF };
		inline constexpr Color DarkGray { 0x64, 0x64, 0x64, 0xFF };
		inline constexpr Color Transparent { 0xFF, 0xFF, 0xFF, 0x00 };
		inline constexpr Color Debug { 0xC0, 0x00, 0xC0, 0xFF };
		inline constexpr Color TextForeground { 0x00, 0x00, 0x00, 0xFF };
		inline constexpr Color TextSelectionForeground { 0xFF, 0xFF, 0xFF, 0xFF };
		inline constexpr Color TextSelectionBackground { 0x99, 0xC9, 0xEF, 0xFF };
		inline constexpr Color AppBackground { 0xfa, 0xf9, 0xf5, 255 };
		inline constexpr Color SidePanelBackground { 0xEE, 0xEC, 0xE4, 0xFF };
		inline constexpr Color SidePanelGradient { 0xe0, 0xdc, 0xcb, 0xFF };
		inline constexpr Color ChatBackground { 0xfa, 0xf9, 0xf5, 255 };
		inline constexpr Color MessageBorderDefault { 0x9f, 0x9f, 0x9f, 0xff };
		inline constexpr Color MessageBackgroundDefault { 0xf4, 0xf4, 0xf4, 0xff };
		inline constexpr Color MessageBorderBlue { 0x4d, 0xa1, 0xc1, 0xff };
		inline constexpr Color MessageBackgroundBlue { 0xf2, 0xfb, 0xff, 0xff };
		inline constexpr Color MessageBorderPink { 0xef, 0x76, 0xbd, 0xff };
		inline constexpr Color MessageBackgroundPink { 0xff, 0xf3, 0xf9, 0xff };
		inline constexpr Color MessageBorderGreen { 0x50, 0xe4, 0x33, 0xff };
		inline constexpr Color MessageBackgroundGreen { 0xea, 0xff, 0xe9, 0xff };
		inline constexpr Color MessageBorderYellow { 0xe4, 0xc5, 0x33, 0xff };
		inline constexpr Color MessageBackgroundYellow { 0xff, 0xfc, 0xea, 0xff };
		inline constexpr Color MessageBorderRed { 0xd5, 0x2b, 0x2b, 0xff };
		inline constexpr Color MessageBackgroundRed { 0xff, 0xee, 0xee, 0xff };
		inline constexpr Color MessageBorderTeal { 0x4d, 0xc1, 0xba, 0xff };
		inline constexpr Color MessageBackgroundTeal { 0xee, 0xff, 0xfc, 0xff };
		inline constexpr Color MessageBorderPurple { 0xb2, 0x5c, 0xe1, 0xff };
		inline constexpr Color MessageBackgroundPurple { 0xfc, 0xf3, 0xff, 0xff };
		inline constexpr Color MessageBorderBrown { 0xc0, 0x7c, 0x4c, 0xff };
		inline constexpr Color MessageBackgroundBrown { 0xff, 0xf9, 0xea, 0xff };
		inline constexpr Color MessageBorderNavy { 0x4d, 0x55, 0xc1, 0xff };
		inline constexpr Color MessageBackgroundNavy { 0xf2, 0xfa, 0xff, 0xff };

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

inline constexpr fig::gui::Colorf to_colorf(fig::gui::Color color)
{
	return fig::gui::Colorf {
		color.r / 255.0f,
		color.g / 255.0f,
		color.b / 255.0f,
		color.a / 255.0f,
	};
}

inline constexpr fig::gui::Color to_color(fig::gui::Colorf color)
{
	return fig::gui::Color {
		std::clamp(static_cast<uint8_t>(color.r * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.g * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.b * 255.0f), 0_u8, 255_u8),
		std::clamp(static_cast<uint8_t>(color.a * 255.0f), 0_u8, 255_u8),
	};
}