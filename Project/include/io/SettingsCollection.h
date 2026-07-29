#pragma once

#include "Figment.h"
#include "util/Hash.h"

namespace fig::io
{
	struct SettingKey 
	{
		fig::string_view section;
		fig::string_view key;

		auto operator<=>(const SettingKey&) const = default;
	};
	using SettingValue = std::variant<bool, int32_t, fig::fixed, fig::point, fig::string, std::vector<fig::fixed>, std::vector<fig::string>>;
	using SettingTuple = std::tuple<SettingKey, SettingValue>;

	class SettingsCollection
	{
		SettingsCollection() = delete;
		SettingsCollection(const SettingsCollection& other) = delete;
	public:
		explicit SettingsCollection(const fig::path& path) noexcept :
			_filename { path }
		{};

		virtual void Init() noexcept = 0;
		virtual FileError Load() noexcept = 0;
		virtual FileError Save() const noexcept = 0;

		bool GetBool(SettingKey key, bool defaultValue = false) const noexcept
		{
			return GetValue<bool>(key, defaultValue);
		}

		int32_t GetInt(SettingKey key, int32_t defaultValue = 0) const noexcept
		{
			return GetValue<int32_t>(key, defaultValue);
		}

		fig::point GetPoint2D(SettingKey key, const fig::point& defaultValue = {}) const noexcept
		{
			return GetValue<fig::point>(key, defaultValue);
		}

		template <size_t N>
		std::vector<int32_t> GetIntVector(SettingKey key, const std::vector<int32_t>& defaultValue = {}) const noexcept
		{
			std::vector<fig::fixed> defaultFixed = defaultValue
				| std::views::transform([](auto& v) { return toFixed(v); })
				| std::ranges::to<std::vector>();

			std::vector<int32_t> vec = GetFixedVector<N>(key, defaultFixed)
				| std::views::transform([](auto& v) { return static_cast<int32_t>(v); })
				| std::ranges::to<std::vector>();
			return vec;
		}

		fig::fixed GetFixed(SettingKey key, fig::fixed defaultValue = 0_fp) const noexcept
		{
			return GetValue<fig::fixed>(key, defaultValue);
		}

		template <size_t N>
		std::vector<fig::fixed> GetFixedVector(SettingKey key, const std::vector<fig::fixed>& defaultValue = {}) const noexcept
		{
			auto vec = GetValue<std::vector<fig::fixed>>(key, defaultValue);
			vec.resize(N, 0_fp);
			return vec;
		}

		fig::string GetString(SettingKey key, const fig::string& defaultValue = "") const noexcept
		{
			return GetValue<fig::string>(key, defaultValue);
		}

		fig::uuid GetUUID(SettingKey key, const fig::uuid& defaultValue = {}) const noexcept
		{
			fig::string strDefault { defaultValue.to_str() };
			return fig::uuid::from_str(GetValue<fig::string>(key, strDefault).c_str());
		}

		template<typename T>
		T GetEnum(SettingKey key, T defaultValue) const noexcept
		{
			return static_cast<T>(GetInt(key, static_cast<int32_t>(defaultValue)));
		}

		template <typename K, typename T, size_t N>
			requires std::is_enum_v<K> and std::constructible_from<fig::string, T>
		K GetEnum(SettingKey key, const std::array<std::pair<K, T>, N>& mapping, const K& default_value = {}) const noexcept
		{
			auto value = GetString(key, {});
			return enum_deserialize(value, mapping, default_value);
		}

		template<typename F, typename T = EnumFlags<F>>
		T GetFlags(SettingKey key, T defaultValue) const noexcept
		{
			auto value = GetInt(key, static_cast<int32_t>(defaultValue.ToRaw()));
			return T::FromRaw(static_cast<T::UnderlyingType>(value));
		}

		template<typename F, typename T = EnumFlags<F>, std::ranges::range Map>
		T GetFlags(SettingKey key, T defaultValue, const Map& mapping) const noexcept
		{
			auto value = GetValue<std::vector<fig::string>>(key, T::Serialize(defaultValue, mapping));
			return T::Deserialize(value, mapping);
		}

		void SetBool(SettingKey key, bool value) noexcept
		{
			SetValue<bool>(key, value);
		}

		void SetInt(SettingKey key, int32_t value) noexcept
		{
			return SetValue<int32_t>(key, value);
		}

		void SetIntVector(SettingKey key, const std::vector<int32_t>& value) noexcept
		{
			SetFixedVector(key, value
				| std::views::transform([](auto& v) { return toFixed(v); })
				| std::ranges::to<std::vector>());
		}

		void SetFixed(SettingKey key, fig::fixed value) noexcept
		{
			return SetValue<fig::fixed>(key, value);
		}

		void SetFixedVector(SettingKey key, const std::vector<fig::fixed>& value) noexcept
		{
			SetValue<std::vector<fig::fixed>>(key, value);
		}

		void SetString(SettingKey key, const fig::string& value) noexcept
		{
			return SetValue<fig::string>(key, value);
		}

		void SetUUID(SettingKey key, const fig::uuid& value) noexcept
		{
			return SetValue<fig::string>(key, value.to_str());
		}

		template<typename T>
		void SetEnum(SettingKey key, T value) noexcept
		{
			return SetInt(key, static_cast<int32_t>(value));
		}

		template <typename K, typename T, std::size_t N>
			requires std::is_enum_v<K> and std::constructible_from<fig::string, T>
		void SetEnum(SettingKey key, const fig::string& value, const std::array<std::pair<K, T>, N>& map) noexcept
		{
			return SetValue<fig::string>(key, enum_serialize(value, map));
		}

		template<typename F, typename T = EnumFlags<F>>
		void SetFlags(SettingKey key, T value) noexcept
		{
			return SetInt(key, static_cast<int32_t>(value.ToRaw()));
		}

		template<typename F, typename T = EnumFlags<F>, std::ranges::range Map>
		void SetFlags(SettingKey key, T value, const Map& mapping) noexcept
		{
			return SetValue<std::vector<fig::string>>(key, value.Serialized(mapping));
		}

	protected:
		void OnInit(const std::vector<SettingTuple>& settings) noexcept;
		FileError OnLoad(const std::vector<SettingTuple>& settings) noexcept;
		FileError OnSave(const std::vector<SettingTuple>& settings) const noexcept;

		template <typename T>
		T GetValue(SettingKey key, const T& defaultValue) const noexcept
		{
			if (auto itFind = _values.find(key); itFind != _values.end())
			{
				if (const T* pValue = std::get_if<T>(&itFind->second))
					return T { *pValue };
			}
			return defaultValue;
		}

		template <typename T>
		void SetValue(SettingKey key, const T& value) noexcept
		{
			_values[key] = value;
		}

		fig::path _filename {};
		std::map<SettingKey, SettingValue> _values {};
	};
}
