#pragma once

#include "gui/ColorRef.h"
#include <io/Error.h>

namespace fig::gui
{
	using ColorTable = std::array<fig::color, static_cast<size_t>(Color::Count)>;

	enum class Theme
	{
		SystemDefault = 0,
		LightDefault,
		LightPink,
		LightBlue,
		LightGreen,
		DarkDefault,
		DarkPink,
		DarkBlue,
		DarkGreen,
		DarkBrown,

		Count,
	};

	constexpr auto ColorThemeMapping = std::array<std::pair<Theme, std::string_view>, static_cast<size_t>(Theme::Count)> {
		std::pair { Theme::SystemDefault,	"Default" },
		std::pair { Theme::LightDefault,	"Light" },
		std::pair { Theme::LightPink,		"LightPink" },
		std::pair { Theme::LightBlue,		"LightBlue" },
		std::pair { Theme::LightGreen,		"LightGreen" },
		std::pair { Theme::DarkDefault,		"Dark" },
		std::pair { Theme::DarkPink,		"DarkPink" },
		std::pair { Theme::DarkBlue,		"DarkBlue" },
		std::pair { Theme::DarkGreen,		"DarkGreen" },
		std::pair { Theme::DarkBrown,		"DarkBrown" },
	};

	class ColorTheme
	{
	public:
		static bool Init();
		static bool SetTheme(Theme theme, bool bTransition = false);
		static Theme GetTheme() noexcept { return _state.currentTheme; }
		static void Update(float fElapsed) noexcept;
		static bool IsTransitioning() noexcept { return _state.isTransitioning; }

	private:
		static fig::io::FileError LoadColorTheme(Theme theme, const fig::path& path);

		static struct State
		{
			ColorTable colorTable {};
			std::map<Theme, ColorTable> colorThemes {};
			Theme currentTheme { Theme::SystemDefault };

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

}