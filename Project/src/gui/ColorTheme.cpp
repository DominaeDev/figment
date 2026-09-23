#include <pch.h>
#include "gui/ColorTheme.h"
#include "util/CommonUtils.h"
#include "io/FileUtility.h"

using namespace fig::io;

namespace fig::gui
{
	static constexpr fig::color MissingColor { 0xFE0000FE_rgba };
	static constexpr fig::color DebugColor { 0xC000C0_rgb };
	static constexpr float TransitionDuration = 0.35f;

	ColorTheme::State ColorTheme::_state {};

	static void ParseTable(ColorTable& table, fig::string_view text)
	{
		table[static_cast<size_t>(Color::Undefined)] = 0x00_rgba;

		for (auto row : std::views::split(text, '\n'))
		{
			auto line = trim(fig::string_view { row });
			if (line.empty())
				continue;

			// Ignore comments
			if (line.starts_with("//"))
				continue;
			size_t pos_comment = line.find("//");
			if (pos_comment != fig::string_view::npos)
				line = trim(line.substr(0, pos_comment));

			size_t pos_name_end = line.find_first_of(" \t");
			fig::string_view name = trim(line.substr(0, pos_name_end));
			size_t pos_value_begin = line.find_first_not_of(" \t", pos_name_end);
			fig::string_view value = trim(line.substr(pos_value_begin));

#if defined(__INTELLISENSE__)
#pragma diag_suppress 304,65 // Suppress bugged Intellisense error squigglies
#endif
			if (auto enumColor = enum_deserialize(name, ColorNameMapping, Color::Undefined); enumColor != Color::Undefined)
				table[static_cast<size_t>(enumColor)] = fig::color::FromString(value);
		}
	}

	bool ColorTheme::Init()
	{
		if constexpr (Debugging)
		{
			// Fill tables with "error color" (red)
			for (int32_t theme = 0; theme < static_cast<int32_t>(Theme::Count); ++theme)
			{
				auto& table = _state.colorThemes[static_cast<Theme>(theme)];
				for (auto& color : table)
					color = MissingColor;
			}
		}

		LoadColorTheme(Theme::LightDefault,		fig::path { "resources/gui/themes/light_default.txt" });
		LoadColorTheme(Theme::LightPink, 		fig::path { "resources/gui/themes/light_pink.txt" });
		LoadColorTheme(Theme::LightBlue,		fig::path { "resources/gui/themes/light_blue.txt" });
		LoadColorTheme(Theme::LightGreen,		fig::path { "resources/gui/themes/light_green.txt" });
		LoadColorTheme(Theme::DarkDefault,		fig::path { "resources/gui/themes/dark_default.txt" });
		LoadColorTheme(Theme::DarkPink,			fig::path { "resources/gui/themes/dark_pink.txt" });
		LoadColorTheme(Theme::DarkBlue,			fig::path { "resources/gui/themes/dark_blue.txt" });
		LoadColorTheme(Theme::DarkGreen,		fig::path { "resources/gui/themes/dark_green.txt" });
		LoadColorTheme(Theme::DarkBrown,		fig::path { "resources/gui/themes/dark_brown.txt" });

		return SetTheme(Theme::SystemDefault);
	}

	FileError ColorTheme::LoadColorTheme(Theme theme, const fig::path& path)
	{
		if (auto file = ReadTextFile(path, false))
		{
			ParseTable(_state.colorThemes[theme], *file);
			return FileError::NoError;
		}
		return FileError::NotFound;
	}

	bool ColorTheme::SetTheme(Theme theme, bool bTransition)
	{
		if (theme == Theme::SystemDefault)
		{
			switch (SDL_GetSystemTheme())
			{
			default:
			case SDL_SYSTEM_THEME_LIGHT: 
				theme = Theme::LightDefault;
				break;
			case SDL_SYSTEM_THEME_DARK: 
				theme = Theme::DarkDefault;
				break;
			}
		}

		auto& table = _state.colorThemes[theme];
		_state.currentTheme = theme;

		if (bTransition)
		{
			// Set up transition
			_state.isTransitioning = bTransition;
			_state.transitionTimer = 0.0f;
			std::copy(_state.colorTable.cbegin(), _state.colorTable.cend(), _state.fromTable.begin());
			std::copy(table.cbegin(), table.cend(), _state.toTable.begin());
			return true; //! @todo: Handle error
		}
		else
		{
			// Change table
			std::copy(table.cbegin(), table.cend(), _state.colorTable.begin());
			return (uint32_t)_state.colorTable[0uz] != (uint32_t)MissingColor;
		}
	}

	static std::map<uint32_t, fig::color> _CustomColors {};
	fig::color_ref custom_color(const fig::color& color)
	{
		auto& c = _CustomColors[static_cast<uint32_t>(color)] = color;

		if constexpr (Debugging and Disabled)
		{
			c = DebugColor;
		}

		return fig::color_ref(&c);
	}

	void ColorTheme::Update(float fElapsed) noexcept
	{
		if (not _state.isTransitioning)
			return;

		_state.transitionTimer += fElapsed / TransitionDuration;
		if (_state.transitionTimer >= 1.0f)
		{
			std::copy(_state.toTable.cbegin(), _state.toTable.cend(), _state.colorTable.begin());
			_state.isTransitioning = false;
			return;
		}

		for (size_t i = 0uz; i < _state.colorTable.size(); ++i)
			_state.colorTable[i] = _state.fromTable[i].Blend(_state.toTable[i], _state.transitionTimer * _state.transitionTimer);
	}
}