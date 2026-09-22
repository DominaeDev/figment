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

		Default = Light,
	};

	constexpr auto ColorThemeMapping = std::array<std::pair<ColorTheme, std::string_view>, 2uz> {
		std::pair { ColorTheme::Light,	"Light" },
		std::pair { ColorTheme::Dark,	"Dark" },
	};

	using ColorTable = std::array<fig::color, static_cast<size_t>(Color::Count)>;
	
	extern ColorTable _ColorTable;
	extern std::map<ColorTheme, ColorTable> _ColorThemes;
	extern std::map<uint32_t, fig::color> _CustomColors;

	extern void InitColorThemes();
	extern fig::io::FileError LoadColorTheme(ColorTheme theme, const fig::path& path);
	extern void ApplyColorTheme(ColorTheme theme);
	extern void CycleColors();

	inline fig::color_ref custom_color(const fig::color& color)
	{
		auto& c = _CustomColors[static_cast<uint32_t>(color)] = color;

		if constexpr (Debugging and Disabled)
		{
			c = 0xc000c0_rgb; // Debug color
		}
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

	struct color_set
	{
		color_ref background {};
		color_ref foreground {};
		color_ref border {};
	};

	struct color_pair
	{
		color_ref background {};
		color_ref foreground {};
	};

	struct ButtonTheme
	{
		color_set defaultColor;
		color_set hoverColor;
		color_set pressedColor;
		color_set disabledColor;
	};

	namespace Theme
	{
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
}