#include <pch.h>
#include "gui/ColorTheme.h"
#include "util/CommonUtils.h"
#include "io/FileUtility.h"
#include "io/CsvFile.h"

using namespace fig::io;

namespace fig::gui
{
	static constexpr fig::color MissingColor { 0xFE00FEFE_rgba };
	static constexpr fig::color DebugColor { 0xC000C0_rgb };
	static constexpr float TransitionDuration = 0.35f;

	AppColors::State AppColors::_state {};

	bool AppColors::Init()
	{
		if constexpr (Debugging)
		{
			// Fill tables with "error color" (red)
			for (int32_t theme = 0; theme < ThemeCount(); ++theme)
			{
				auto& table = _state.colorThemes[static_cast<ColorTheme>(theme)];
				for (auto& color : table)
					color = MissingColor;
			}
		}

		LoadColorThemesCSV(fig::path { "resources/gui/themes/themes.csv" });

		return SetTheme(ColorTheme::SystemDefault);
	}

	bool AppColors::SetTheme(ColorTheme theme, bool bTransition)
	{
		if (theme == ColorTheme::SystemDefault)
		{
			switch (SDL_GetSystemTheme())
			{
			default:
			case SDL_SYSTEM_THEME_LIGHT: 
				theme = ColorTheme::LightDefault;
				break;
			case SDL_SYSTEM_THEME_DARK: 
				theme = ColorTheme::DarkDefault;
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

	void AppColors::Update(float fElapsed) noexcept
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

	fig::color_ref opacity(float fAlpha)
	{
		return custom_color(0xFFFFFF_rgb).WithAlpha(fAlpha);
	}

	FileError AppColors::LoadColorThemesCSV(const fig::path& filename)
	{
		CsvFile csv;
		if (csv.Load(filename) != CsvError::NoError)
			return FileError::ReadError;

		if (csv.GetColumnCount() < ThemeCount() + 1uz)
			return FileError::ReadError;

		for (size_t iTheme = 0uz; iTheme < ThemeCount(); ++iTheme)
		{
			ColorTheme theme = static_cast<ColorTheme>(toUZ(MinTheme) + iTheme);
			auto& table = _state.colorThemes[theme];
			table[static_cast<size_t>(Color::Undefined)] = 0x00_rgba;

			for (auto& row : csv.GetRows())
			{
				if (row.empty())
					continue;

				auto& colorName = row[0];
				if (empty_or_whitespace(colorName))
					continue;


				if (auto enumColor = enum_deserialize(colorName, ColorNameMapping, Color::Undefined); enumColor != Color::Undefined)
					table[static_cast<size_t>(enumColor)] = fig::color::FromString(row[1uz + iTheme]);
			}
		}
		return FileError::NoError;
	}

}