#include <pch.h>
#include "gui/ColorTable.h"
#include "util/CommonUtils.h"
#include "io/FileUtility.h"

using namespace fig::io;

namespace fig::gui
{
	ColorTable _ColorTable {};
	std::map<ColorTheme, ColorTable> _ColorThemes {};
	std::map<uint32_t, fig::color> _CustomColors {};

	void ParseTable(ColorTable& table, fig::string_view text)
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

	FileError LoadColorTheme(ColorTheme theme, const fig::path& path)
	{
		if (auto file = ReadTextFile(path, false))
		{
			ParseTable(_ColorThemes[theme], *file);
			return FileError::NoError;
		}
		return FileError::NotFound;
	}

	void InitColorThemes()
	{
		if constexpr (Debugging)
		{
			// Fill tables with "error color" (red)
			auto themes = { ColorTheme::Light, ColorTheme::Dark };
			for (auto theme : themes)
			{
				auto& table = _ColorThemes[theme];
				for (auto& color : table)
					color = 0xFF0000_rgb;
			}
		}

		LoadColorTheme(ColorTheme::Light, fig::path { "resources/gui/themes/light.txt" });
		LoadColorTheme(ColorTheme::Dark, fig::path { "resources/gui/themes/dark.txt" });
	}

	void ApplyColorTheme(ColorTheme theme)
	{
		auto& table = _ColorThemes[theme];
		std::copy(table.cbegin(), table.cend(), _ColorTable.begin());
	}

	void CycleColors()
	{
		if constexpr (Enabled and Debugging) //! @temp
		{
			for (auto& color : _ColorTable)
			{
				color.r = color.r + 1;
				color.g = color.g - 1;
				color.b = color.b + 2;
			}

			for (auto& kvp : _CustomColors)
			{
				kvp.second.r = kvp.second.r + 1;
				kvp.second.g = kvp.second.g - 1;
				kvp.second.b = kvp.second.b + 2;
			}
		}
	}
}