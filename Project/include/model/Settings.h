#ifndef SETTINGS_H__
#define SETTINGS_H__
#pragma once

#include "Types.h"
#include <variant>

enum class TestAppSetting
{
	LastUser,
	WindowMaximized,
	WindowSize,
};

namespace fig
{
	template <typename E>
	concept IsEnum = std::is_enum_v<E>;

	using SettingValue = std::variant<int32_t, float, fig::string, std::vector<int32_t>, std::vector<float>, std::vector<fig::string>>;

	template <IsEnum E>
	class SettingsCollection
	{
	public:
		explicit SettingsCollection(const fig::path& path) noexcept :
			_filename { path }
		{
			Init();
		}

		void Init() noexcept;
		bool Load() noexcept;
		bool Save() noexcept;

		bool GetBool(E setting, bool defaultValue = false) const noexcept
		{
			return GetValue<int32_t>(setting, defaultValue) != 0;
		}

		int32_t GetInt(E setting, int32_t defaultValue = 0) const noexcept
		{
			return GetValue<int32_t>(setting, defaultValue);
		}

		template <size_t N>
		std::vector<int32_t> GetIntVector(E setting, const std::vector<int32_t>& defaultValue = {}) const noexcept
		{
			std::vector<int32_t> vec = GetValue<std::vector<int32_t>>(setting, {});
			vec.resize(N);
			return vec;
		}

		float GetFloat(E setting, float defaultValue = 0.0f) const noexcept
		{
			return GetValue<float>(setting, defaultValue);
		}

		template <size_t N>
		std::vector<float> GetFloatVector(E setting, const std::vector<float>& defaultValue = {}) const noexcept
		{
			auto vec = GetValue<std::vector<float>>(setting, defaultValue);
			vec.resize(N);
			return vec;
		}

		fig::string GetString(E setting, const fig::string& defaultValue = "") const noexcept
		{
			return GetValue<fig::string>(setting, defaultValue);
		}

		fig::uuid GetUUID(E setting, const fig::uuid& defaultValue = {}) const noexcept
		{
			fig::string strDefault { defaultValue.str() };
			return fig::uuid { GetValue<fig::string>(setting, strDefault).c_str() };
		}

		void SetBool(E setting, bool value) noexcept
		{
			SetValue<int32_t>(setting, value ? 1 : 0);
		}

		void SetInt(E setting, int32_t value) noexcept
		{
			return SetValue<int32_t>(setting, value);
		}

		void SetFloat(E setting, float value) noexcept
		{
			return SetValue<float>(setting, value);
		}

		void SetString(E setting, const fig::string& value) noexcept
		{
			return SetValue<fig::string>(setting, value);
		}

		void SetUUID(E setting, const fig::uuid& value) noexcept
		{
			return SetValue<fig::uuid>(setting, value);
		}

	private:
		template <typename T>
		T GetValue(E setting, const T& defaultValue) const noexcept
		{
			if (auto itFind = _values.find(setting); itFind != _values.end())
			{
				if (const T* pValue = std::get_if<T>(&itFind->second))
					return T { *pValue };
			}
			return defaultValue;
		}

		template <typename T>
		void SetValue(E setting, const T& value) noexcept
		{
			_values[setting] = value;
		}

		fig::path _filename {};
		std::map<E, SettingValue> _values {};
	};
}

#endif
