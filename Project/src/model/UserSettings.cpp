#include <pch.h>
#include "model/UserSettings.h"
#include "util/IniFile.h"

	using namespace fig::util;

	namespace fig
	{
		using SettingTuple = std::tuple<UserSetting, fig::string, fig::string, SettingValue>;

		static const std::vector<SettingTuple> _AllSettings {
			std::tuple { UserSetting::ShowTags,		"Interface",	"ShowTags",		1 },
		};

		void SettingsCollection<UserSetting>::Init() noexcept
		{
			for (auto& [setting, _x, _y, defaultValue] : _AllSettings)
				SetValue(setting, defaultValue);
		}

		bool SettingsCollection<UserSetting>::Load() noexcept
		{
			IniFile ini;
			if (auto load = ini.Load(_filename))
			{
				for (auto& [setting, section, key, defaultValue] : _AllSettings)
				{
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

		static std::pair<fig::string, fig::string> GetSectionKey(UserSetting setting)
		{
			if (auto itFind = std::ranges::find_if(_AllSettings, [setting](const auto& t) { return std::get<0>(t) == setting; }); itFind != std::end(_AllSettings))
				return std::make_pair(std::get<1>(*itFind), std::get<2>(*itFind));
			return {};
		}

		bool SettingsCollection<UserSetting>::Save() noexcept
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