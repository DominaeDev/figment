#pragma once

#include "Figment.h"
#include "gui/ColorRef.h"
#include "io/Error.h"

namespace fig::gui
{
	enum class ColorTheme
	{
		Light = 0,
		Dark,
	};

	enum class Color
	{
		Invalid,
		Debug,
		Debug2,
		Debug3,
		Opaque,
		Transparent,
		White,
		Black,
		Red,
		Green,
		Blue,
		DarkGray,
		TextForeground,
		TextSelectionForeground,
		TextSelectionBackground,
		TextSelectionBackgroundInactive,
		AppBackground,
		DisabledForeground,
		DisabledBackground,
		SidePanelForeground,
		SidePanelBackground,
		SidePanelGradient,
		LineColor,
		DisabledLineColor,
		Icon,
		TextBoxForeground,
		TextBoxBackground,
		TextBoxScrollBar,
		StatusBarForeground,
		StatusBarBackground,
		GenderTagMale,
		GenderTagFemale,
		GenderTagOther,
		ChatBackground,
		MenuBackgroundColor,
		MenuBorderColor,
		MenuItemHoverColor,
		MenuItemPressedColor,
		MessageBorderDefault,
		MessageBackgroundDefault,
		MessageBorderBlue,
		MessageBackgroundBlue,
		MessageBorderPink,
		MessageBackgroundPink,
		MessageBorderGreen,
		MessageBackgroundGreen,
		MessageBorderYellow,
		MessageBackgroundYellow,
		MessageBorderRed,
		MessageBackgroundRed,
		MessageBorderTeal,
		MessageBackgroundTeal,
		MessageBorderPurple,
		MessageBackgroundPurple,
		MessageBorderBrown,
		MessageBackgroundBrown,
		MessageBorderNavy,
		MessageBackgroundNavy,

		Count,
	};
	
	constexpr auto _ColorNameMapping = std::array<std::pair<Color, std::string_view>, static_cast<size_t>(Color::Count)> {
		std::pair { Color::Debug,								"Debug" },
		std::pair { Color::Debug2,								"Debug2" },
		std::pair { Color::Debug3,								"Debug3" },
		std::pair { Color::Opaque,								"Opaque" },
		std::pair { Color::Transparent,						"Transparent" },
		std::pair { Color::White,								"White" },
		std::pair { Color::Black,								"Black" },
		std::pair { Color::Red,								"Red" },
		std::pair { Color::Green,								"Green" },
		std::pair { Color::Blue,								"Blue" },
		std::pair { Color::DarkGray,							"DarkGray" },
		std::pair { Color::TextForeground,						"TextForeground" },
		std::pair { Color::TextSelectionForeground,				"TextSelectionForeground" },
		std::pair { Color::TextSelectionBackground,				"TextSelectionBackground" },
		std::pair { Color::TextSelectionBackgroundInactive,		"TextSelectionBackgroundInactive" },
		std::pair { Color::AppBackground,						"AppBackground" },
		std::pair { Color::DisabledForeground,					"DisabledForeground" },
		std::pair { Color::DisabledBackground,					"DisabledBackground" },
		std::pair { Color::SidePanelForeground,					"SidePanelForeground" },
		std::pair { Color::SidePanelBackground,					"SidePanelBackground" },
		std::pair { Color::SidePanelGradient,					"SidePanelGradient" },
		std::pair { Color::LineColor,							"LineColor" },
		std::pair { Color::DisabledLineColor,					"DisabledLineColor" },
		std::pair { Color::Icon,								"Icon" },
		std::pair { Color::TextBoxForeground,					"TextBoxForeground" },
		std::pair { Color::TextBoxBackground,					"TextBoxBackground" },
		std::pair { Color::TextBoxScrollBar,					"TextBoxScrollBar" },
		std::pair { Color::StatusBarForeground,					"StatusBarForeground" },
		std::pair { Color::StatusBarBackground,					"StatusBarBackground" },
		std::pair { Color::GenderTagMale,						"GenderTagMale" },
		std::pair { Color::GenderTagFemale,						"GenderTagFemale" },
		std::pair { Color::GenderTagOther,						"GenderTagOther" },
		std::pair { Color::ChatBackground,						"ChatBackground" },
		std::pair { Color::MenuBackgroundColor,					"MenuBackgroundColor" },
		std::pair { Color::MenuBorderColor,						"MenuBorderColor" },
		std::pair { Color::MenuItemHoverColor,					"MenuItemHoverColor" },
		std::pair { Color::MenuItemPressedColor,				"MenuItemPressedColor" },
		std::pair { Color::MessageBorderDefault,				"MessageBorderDefault" },
		std::pair { Color::MessageBackgroundDefault,			"MessageBackgroundDefault" },
		std::pair { Color::MessageBorderBlue,					"MessageBorderBlue" },
		std::pair { Color::MessageBackgroundBlue,				"MessageBackgroundBlue" },
		std::pair { Color::MessageBorderPink,					"MessageBorderPink" },
		std::pair { Color::MessageBackgroundPink,				"MessageBackgroundPink" },
		std::pair { Color::MessageBorderGreen,					"MessageBorderGreen" },
		std::pair { Color::MessageBackgroundGreen,				"MessageBackgroundGreen" },
		std::pair { Color::MessageBorderYellow,					"MessageBorderYellow" },
		std::pair { Color::MessageBackgroundYellow,				"MessageBackgroundYellow" },
		std::pair { Color::MessageBorderRed,					"MessageBorderRed" },
		std::pair { Color::MessageBackgroundRed,				"MessageBackgroundRed" },
		std::pair { Color::MessageBorderTeal,					"MessageBorderTeal" },
		std::pair { Color::MessageBackgroundTeal,				"MessageBackgroundTeal" },
		std::pair { Color::MessageBorderPurple,					"MessageBorderPurple" },
		std::pair { Color::MessageBackgroundPurple,				"MessageBackgroundPurple" },
		std::pair { Color::MessageBorderBrown,					"MessageBorderBrown" },
		std::pair { Color::MessageBackgroundBrown,				"MessageBackgroundBrown" },
		std::pair { Color::MessageBorderNavy,					"MessageBorderNavy" },
		std::pair { Color::MessageBackgroundNavy,				"MessageBackgroundNavy" },
	};

	using ColorTable = std::array<fig::color, static_cast<size_t>(Color::Count)>;
	
	extern ColorTable _ColorTable;
	extern std::map<ColorTheme, ColorTable> _ColorThemes;
	extern std::map<uint32_t, fig::color> _CustomColors;

	extern fig::io::FileError LoadColorTheme(ColorTheme theme, const fig::path& path);
	extern void ApplyColorTheme(ColorTheme theme);
	extern void CycleColors();

	inline fig::color_ref custom_color(const fig::color& color)
	{
		auto& c = _CustomColors[static_cast<uint32_t>(color)] = color;
		return fig::color_ref(&c);
	}

	inline static fig::color_ref DefaultUserMessageBorder { Color::MessageBorderBlue };
	inline static fig::color_ref DefaultUserMessageBackground { Color::MessageBackgroundBlue };

	inline static std::array<fig::color_ref, 8> DefaultBotMessageBorders {
		Color::MessageBorderPink,
		Color::MessageBorderGreen,
		Color::MessageBorderYellow,
		Color::MessageBorderRed,
		Color::MessageBorderTeal,
		Color::MessageBorderPurple,
		Color::MessageBorderBrown,
		Color::MessageBorderNavy,
	};

	inline static std::array<fig::color_ref, 8> DefaultBotMessageBackgrounds {
		Color::MessageBackgroundPink,
		Color::MessageBackgroundGreen,
		Color::MessageBackgroundYellow,
		Color::MessageBackgroundRed,
		Color::MessageBackgroundTeal,
		Color::MessageBackgroundPurple,
		Color::MessageBackgroundBrown,
		Color::MessageBackgroundNavy,
	};

}