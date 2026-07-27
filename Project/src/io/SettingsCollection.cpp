#include <pch.h>
#include "io/SettingsCollection.h"
#include "io/IniFile.h"

namespace fig::io
{
	void SettingsCollection::OnInit(const std::vector<SettingTuple>& settings) noexcept
	{
		for (auto& [key, defaultValue] : settings)
			SetValue(key, defaultValue);
	}

	FileError SettingsCollection::OnLoad(const std::vector<SettingTuple>& settings) noexcept
	{
		IniFile ini;
		if (auto try_load = ini.Load(_filename))
		{
			for (auto& [key, defaultValue] : settings)
			{
				if (ini.HasKey(key.section, key.key))
				{
					if (std::holds_alternative<bool>(defaultValue))
						SetValue(key, string_to_bool(ini.Get<fig::string>(key.section, key.key).value_or(""), std::get<bool>(defaultValue)));
					else if (std::holds_alternative<int32_t>(defaultValue))
						SetValue(key, static_cast<int32_t>(ini.Get<fig::fixed>(key.section, key.key).value_or(toFixed(std::get<int32_t>(defaultValue)))));
					else if (std::holds_alternative<fig::fixed>(defaultValue))
						SetValue(key, ini.Get<fig::fixed>(key.section, key.key).value_or(std::get<fig::fixed>(defaultValue)));
					else if (std::holds_alternative<fig::string>(defaultValue))
						SetValue(key, ini.Get<fig::string>(key.section, key.key).value_or(std::get<fig::string>(defaultValue)));
					else if (std::holds_alternative<std::vector<fig::fixed>>(defaultValue))
						SetValue(key, ini.Get<std::vector<fig::fixed>>(key.section, key.key).value_or(std::get<std::vector<fig::fixed>>(defaultValue)));
					else if (std::holds_alternative<std::vector<fig::string>>(defaultValue))
						SetValue(key, ini.Get<std::vector<fig::string>>(key.section, key.key).value_or(std::get<std::vector<fig::string>>(defaultValue)));
					else if (std::holds_alternative<fig::point>(defaultValue))
					{
						auto dp = std::get<fig::point>(defaultValue);
						auto load_value = ini.Get<std::vector<fig::fixed>>(key.section, key.key).value_or(std::vector { toFixed(dp.x), toFixed(dp.y) });
						load_value.resize(2);
						SetValue(key, fig::point { static_cast<int32_t>(load_value[0]), static_cast<int32_t>(load_value[1]) });
					}
					else // Fallback
						SetValue(key, ini.Get<fig::string>(key.section, key.key).value_or(""));
				}
			}
			return FileError::NoError;
		}
		else
		{
			switch (try_load.error())
			{
			case IniError::FileNotFound:
				return FileError::NotFound;
			case IniError::FileAccessDenied:
				return FileError::AccessDenied;
			case IniError::FileReadError:
				return FileError::ReadError;
			default:
				return FileError::UnrecognizedFormat;
			}
		}
	}

	FileError SettingsCollection::OnSave(const std::vector<SettingTuple>& settings) const noexcept
	{
		IniFile ini;
		for (auto& kvp : _values)
		{
			auto& [section, key] = kvp.first;
			auto& value = kvp.second;

			if (const bool* x = std::get_if<bool>(&value))
				ini.Set(section, key, *x ? "true" : "false");
			else if (const int32_t* x = std::get_if<int32_t>(&value))
				ini.Set(section, key, toFixed(*x));
			else if (const fig::fixed* x = std::get_if<fig::fixed>(&value))
				ini.Set(section, key, *x);
			else if (const fig::string* x = std::get_if<fig::string>(&value))
				ini.Set(section, key, *x);
			else if (const fig::point* x = std::get_if<fig::point>(&value))
				ini.Set(section, key, std::vector { fig::fixed { (*x).x }, fig::fixed { (*x).y } });
			else if (const std::vector<fig::fixed>* x = std::get_if<std::vector<fig::fixed>>(&value))
				ini.Set(section, key, *x);
			else if (const std::vector<fig::string>* x = std::get_if<std::vector<fig::string>>(&value))
				ini.Set(section, key, *x);
		}

		if (auto try_save = ini.Save(_filename))
			return FileError::NoError;
		else
		{
			switch (try_save.error())
			{
			case IniError::FileAccessDenied:
				return FileError::AccessDenied;
			default:
				return FileError::WriteError;
			}
		}
	}
}