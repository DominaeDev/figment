#pragma once

#include "gui/ColorRef.h"
#include <io/Error.h>

namespace fig::gui
{
	using ColorTable = std::array<fig::color, static_cast<size_t>(Color::Count)>;

	enum class ColorTheme
	{
		SystemDefault = 0,

		LightDefault,
		LightGray,
		LightPink,
		LightBlue,
		LightGreen,
		LightYellow,
		DarkDefault,
		DarkBlack,
		DarkPink,
		DarkBlue,
		DarkGreen,
		DarkBrown,
	};

	constexpr ColorTheme MinTheme = ColorTheme::LightDefault;
	constexpr ColorTheme MaxTheme = ColorTheme::DarkBrown;

	consteval size_t ThemeCount()
	{
		return static_cast<size_t>(MaxTheme) - static_cast<size_t>(MinTheme) + 1uz;
	}

	constexpr auto ColorThemeMapping = std::array<std::pair<ColorTheme, std::string_view>, static_cast<size_t>(ThemeCount() + 1uz)> {
		std::pair { ColorTheme::SystemDefault,	"Default" },
		std::pair { ColorTheme::LightDefault,	"Light" },
		std::pair { ColorTheme::LightGray,		"LightGray" },
		std::pair { ColorTheme::LightPink,		"LightPink" },
		std::pair { ColorTheme::LightBlue,		"LightBlue" },
		std::pair { ColorTheme::LightGreen,		"LightGreen" },
		std::pair { ColorTheme::LightYellow,	"LightYellow" },
		std::pair { ColorTheme::DarkDefault,	"Dark" },
		std::pair { ColorTheme::DarkBlack,		"DarkBlack" },
		std::pair { ColorTheme::DarkPink,		"DarkPink" },
		std::pair { ColorTheme::DarkBlue,		"DarkBlue" },
		std::pair { ColorTheme::DarkGreen,		"DarkGreen" },
		std::pair { ColorTheme::DarkBrown,		"DarkBrown" },
	};

	class AppColors
	{
	public:
		static bool Init();
		static bool SetTheme(ColorTheme theme, bool bTransition = false);
		static ColorTheme GetTheme() noexcept { return _state.currentTheme; }
		static void Update(float fElapsed) noexcept;
		static bool IsTransitioning() noexcept { return _state.isTransitioning; }

	private:
		static fig::io::FileError LoadColorThemesCSV(const fig::path& filename);

		static struct State
		{
			ColorTable colorTable {};
			std::map<ColorTheme, ColorTable> colorThemes {};
			ColorTheme currentTheme { ColorTheme::SystemDefault };

			bool isTransitioning { false };
			ColorTable fromTable {};
			ColorTable toTable {};
			float transitionTimer {};
		} _state;

		friend class fig::color_ref;
		constexpr static fig::color& Get(Color colorKey)
		{
			return _state.colorTable.at(static_cast<size_t>(colorKey));
		}
	};

	extern fig::color_ref custom_color(const fig::color& color);
	extern fig::color_ref opacity(float fAlpha);

}