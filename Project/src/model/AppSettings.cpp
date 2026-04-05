#include <pch.h>
#include "model/AppSettings.h"
#include "util/IniFile.h"

using namespace fig::util;

namespace fig
{
	using SettingTuple = std::tuple<fig::string, fig::string, SettingValue>;

	static const std::unordered_map<AppSetting, SettingTuple> _AllSettings = {
		{ AppSetting::LastUser,			{ "Application", "LastUser", "" } },
		{ AppSetting::SignedIn,			{ "Application", "SignedIn", 0 } },
		{ AppSetting::WindowMaximized,	{ "Application", "WindowMaximized", 0 } },
		{ AppSetting::WindowSize,		{ "Application", "WindowSize", std::vector { Constants::GUI::WindowDefaultWidth, Constants::GUI::WindowDefaultHeight } } },
	};

	void SettingsCollection<AppSetting>::Init() noexcept
	{
		for (auto& [setting, tuple] : _AllSettings)
		{
			auto& [_, __, defaultValue] = tuple;
			SetValue(setting, defaultValue);
		}
	}

	bool SettingsCollection<AppSetting>::Load() noexcept
	{
		IniFile ini;
		if (auto load = ini.Load(_filename))
		{
			for (auto& [setting, tuple] : _AllSettings)
			{
				auto& [section, key, defaultValue] = tuple;
				if (ini.HasKey(section, key))
				{
					if (std::holds_alternative<int32_t>(defaultValue))
						SetValue(setting, ini.Get<int32_t>(section, key).value_or(std::get<int32_t>(defaultValue)));
					else if (std::holds_alternative<float>(defaultValue))
						SetValue(setting, ini.Get<float>(section, key).value_or(std::get<float>(defaultValue)));
					else if (std::holds_alternative<fig::string>(defaultValue))
						SetValue(setting, ini.Get<fig::string>(section, key).value_or(std::get<fig::string>(defaultValue)));
					else if (std::holds_alternative<std::vector<int32_t>>(defaultValue))
						SetValue(setting, ini.Get<std::vector<int32_t>>(section, key).value_or(std::get<std::vector<int32_t>>(defaultValue)));
					else if (std::holds_alternative<std::vector<float>>(defaultValue))
						SetValue(setting, ini.Get<std::vector<float>>(section, key).value_or(std::get<std::vector<float>>(defaultValue)));
					else if (std::holds_alternative<std::vector<fig::string>>(defaultValue))
						SetValue(setting, ini.Get<std::vector<fig::string>>(section, key).value_or(std::get<std::vector<fig::string>>(defaultValue)));
					else // Fallback
						SetValue(setting, ini.Get<fig::string>(section, key).value_or(""));
				}
			}

		}

		return false;
	}

	static std::pair<fig::string, fig::string> GetSectionKey(AppSetting setting)
	{
		if (auto itFind = _AllSettings.find(setting); itFind != std::end(_AllSettings))
			return std::make_pair(std::get<0>(itFind->second), std::get<1>(itFind->second));
		return {};
	}

	bool SettingsCollection<AppSetting>::Save() noexcept
	{
		IniFile ini;
		for (auto& kvp : _values)
		{
			auto [section, key] = GetSectionKey(kvp.first);
			auto& value = kvp.second;

			if (const int32_t* x = std::get_if<int32_t>(&value))
				ini.Set(section, key, *x);
			else if (const float* x = std::get_if<float>(&value))
				ini.Set(section, key, *x);
			else if (const fig::string* x = std::get_if<fig::string>(&value))
				ini.Set(section, key, *x);
			else if (const std::vector<int32_t>* x = std::get_if<std::vector<int32_t>>(&value))
				ini.Set(section, key, *x);
			else if (const std::vector<float>* x = std::get_if<std::vector<float>>(&value))
				ini.Set(section, key, *x);
			else if (const std::vector<fig::string>* x = std::get_if<std::vector<fig::string>>(&value))
				ini.Set(section, key, *x);
		}

		if (auto error = ini.Save(_filename))
			return true;
		return false;
	}
}