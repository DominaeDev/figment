#ifndef SETTINGS_H__
#define SETTINGS_H__
#pragma once

#include "Figment.h"
#include <variant>

namespace fig::io
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
			fig::string strDefault { defaultValue.to_str() };
			return fig::uuid::from_str(GetValue<fig::string>(setting, strDefault).c_str());
		}

		template<typename T>
		T GetEnum(E setting, T defaultValue) const noexcept
		{
			auto value = GetValue<int32_t>(setting, {});
			if (value >= 0 and value < static_cast<int32_t>(T::Count))
				return static_cast<T>(value);
			return defaultValue;
		}

		template<typename F, typename T = EnumFlags<F>>
		T GetFlags(E setting, T defaultValue) const noexcept
		{
			auto value = GetValue<int32_t>(setting, static_cast<int32_t>(defaultValue.ToRaw()));
			return T::FromRaw(static_cast<T::UnderlyingType>(value));
		}

		template<typename F, typename T = EnumFlags<F>, std::ranges::range Map>
		T GetFlags(E setting, T defaultValue, const Map& mapping) const noexcept
		{
			auto value = GetValue<std::vector<fig::string>>(setting, T::Serialize(defaultValue, mapping));
			return T::Deserialize(value, mapping);
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

		void SetFixed(E setting, fig::fixed value) noexcept
		{
			return SetValue<fig::fixed>(setting, value);
		}

		void SetString(E setting, const fig::string& value) noexcept
		{
			return SetValue<fig::string>(setting, value);
		}

		void SetUUID(E setting, const fig::uuid& value) noexcept
		{
			return SetValue<fig::string>(setting, value.to_str());
		}

		template<typename T>
		void SetEnum(E setting, T value) noexcept
		{
			return SetValue<int32_t>(setting, static_cast<int32_t>(value));
		}

		template<typename F, typename T = EnumFlags<F>>
		void SetFlags(E setting, T value) noexcept
		{
			return SetValue<int32_t>(setting, static_cast<int32_t>(value.ToRaw()));
		}

		template<typename F, typename T = EnumFlags<F>, std::ranges::range Map>
		void SetFlags(E setting, T value, const Map& mapping) noexcept
		{
			return SetValue<std::vector<fig::string>>(setting, value.Serialized(mapping));
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
