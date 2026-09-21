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
		if constexpr (Debugging)
		{
			for (auto& color : table)
				color = 0xff00ff_rgb;
		}

		for (auto row : std::views::split(text, '\n'))
		{
			auto line = trim(fig::string_view { row });
			if (line.empty())
				continue;

			size_t pos_name_end = line.find_first_of(" \t");
			fig::string_view name = trim(line.substr(0, pos_name_end));
			size_t pos_value_begin = line.find_first_not_of(" \t", pos_name_end);
			fig::string_view value = trim(line.substr(pos_value_begin));

			if (auto enumColor = enum_deserialize(name, _ColorNameMapping, Color::Invalid); enumColor != Color::Invalid)
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

	void ApplyColorTheme(ColorTheme theme)
	{
		auto& table = _ColorThemes[theme];
		std::copy(table.cbegin(), table.cend(), _ColorTable.begin());
	}

	void CycleColors()
	{
		if constexpr (Disabled and Debugging) //! @temp
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