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

	enum class Colour
	{
		Invalid,
		Debug,
		Debug2,
		Debug3,
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
		StatusBarBackground,
		GenderTagMale,
		GenderTagFemale,
		GenderTagOther,
		ChatBackground,
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
	
	constexpr auto _ColorNameMapping = std::array<std::pair<Colour, std::string_view>, static_cast<size_t>(Colour::Count)> {
		std::pair { Colour::Debug,								"Debug" },
		std::pair { Colour::Debug2,								"Debug2" },
		std::pair { Colour::Debug3,								"Debug3" },
		std::pair { Colour::Transparent,						"Transparent" },
		std::pair { Colour::White,								"White" },
		std::pair { Colour::Black,								"Black" },
		std::pair { Colour::Red,								"Red" },
		std::pair { Colour::Green,								"Green" },
		std::pair { Colour::Blue,								"Blue" },
		std::pair { Colour::DarkGray,							"DarkGray" },
		std::pair { Colour::TextForeground,						"TextForeground" },
		std::pair { Colour::TextSelectionForeground,			"TextSelectionForeground" },
		std::pair { Colour::TextSelectionBackground,			"TextSelectionBackground" },
		std::pair { Colour::TextSelectionBackgroundInactive,	"TextSelectionBackgroundInactive" },
		std::pair { Colour::AppBackground,						"AppBackground" },
		std::pair { Colour::DisabledForeground,					"DisabledForeground" },
		std::pair { Colour::DisabledBackground,					"DisabledBackground" },
		std::pair { Colour::SidePanelForeground,				"SidePanelForeground" },
		std::pair { Colour::SidePanelBackground,				"SidePanelBackground" },
		std::pair { Colour::SidePanelGradient,					"SidePanelGradient" },
		std::pair { Colour::LineColor,							"LineColor" },
		std::pair { Colour::DisabledLineColor,					"DisabledLineColor" },
		std::pair { Colour::Icon,								"Icon" },
		std::pair { Colour::TextBoxForeground,					"TextBoxForeground" },
		std::pair { Colour::TextBoxBackground,					"TextBoxBackground" },
		std::pair { Colour::StatusBarBackground,				"StatusBarBackground" },
		std::pair { Colour::GenderTagMale,						"GenderTagMale" },
		std::pair { Colour::GenderTagFemale,					"GenderTagFemale" },
		std::pair { Colour::GenderTagOther,						"GenderTagOther" },
		std::pair { Colour::ChatBackground,						"ChatBackground" },
		std::pair { Colour::MessageBorderDefault,				"MessageBorderDefault" },
		std::pair { Colour::MessageBackgroundDefault,			"MessageBackgroundDefault" },
		std::pair { Colour::MessageBorderBlue,					"MessageBorderBlue" },
		std::pair { Colour::MessageBackgroundBlue,				"MessageBackgroundBlue" },
		std::pair { Colour::MessageBorderPink,					"MessageBorderPink" },
		std::pair { Colour::MessageBackgroundPink,				"MessageBackgroundPink" },
		std::pair { Colour::MessageBorderGreen,					"MessageBorderGreen" },
		std::pair { Colour::MessageBackgroundGreen,				"MessageBackgroundGreen" },
		std::pair { Colour::MessageBorderYellow,				"MessageBorderYellow" },
		std::pair { Colour::MessageBackgroundYellow,			"MessageBackgroundYellow" },
		std::pair { Colour::MessageBorderRed,					"MessageBorderRed" },
		std::pair { Colour::MessageBackgroundRed,				"MessageBackgroundRed" },
		std::pair { Colour::MessageBorderTeal,					"MessageBorderTeal" },
		std::pair { Colour::MessageBackgroundTeal,				"MessageBackgroundTeal" },
		std::pair { Colour::MessageBorderPurple,				"MessageBorderPurple" },
		std::pair { Colour::MessageBackgroundPurple,			"MessageBackgroundPurple" },
		std::pair { Colour::MessageBorderBrown,					"MessageBorderBrown" },
		std::pair { Colour::MessageBackgroundBrown,				"MessageBackgroundBrown" },
		std::pair { Colour::MessageBorderNavy,					"MessageBorderNavy" },
		std::pair { Colour::MessageBackgroundNavy,				"MessageBackgroundNavy" },
	};

	using ColorTable = std::array<fig::color, static_cast<size_t>(Colour::Count)>;
	
	extern ColorTable _ColorTable;
	extern std::map<ColorTheme, ColorTable> _ColorThemes;
	extern std::map<uint32_t, fig::color> _CustomColors;

	extern fig::color_ref _Color(Colour color);
	extern fig::io::FileError LoadColorTheme(ColorTheme theme, const fig::path& path);
	extern void ApplyColorTheme(ColorTheme theme);

	inline fig::color_ref custom_color(const fig::color& color)
	{
		auto& c = _CustomColors[static_cast<uint32_t>(color)] = color;
		return fig::color_ref(&c);
	}

	inline static fig::color_ref DefaultUserMessageBorder { Colour::MessageBorderBlue };
	inline static fig::color_ref DefaultUserMessageBackground { Colour::MessageBackgroundBlue };

	inline static std::array<fig::color_ref, 8> DefaultBotMessageBorders {
		Colour::MessageBorderPink,
		Colour::MessageBorderGreen,
		Colour::MessageBorderYellow,
		Colour::MessageBorderRed,
		Colour::MessageBorderTeal,
		Colour::MessageBorderPurple,
		Colour::MessageBorderBrown,
		Colour::MessageBorderNavy,
	};

	inline static std::array<fig::color_ref, 8> DefaultBotMessageBackgrounds {
		Colour::MessageBackgroundPink,
		Colour::MessageBackgroundGreen,
		Colour::MessageBackgroundYellow,
		Colour::MessageBackgroundRed,
		Colour::MessageBackgroundTeal,
		Colour::MessageBackgroundPurple,
		Colour::MessageBackgroundBrown,
		Colour::MessageBackgroundNavy,
	};

}