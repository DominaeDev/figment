#pragma once

#include "Types.h"
#include "util/StringUtility.h"

#include <variant>
#include <map>

namespace fig::util
{
	enum class IniError
	{
		NoError,
		FileNotFound,
		FileAccessDenied,
		FileReadError,
		FileWriteError,
		MalformedSection,
		UnclosedMultilineString,
		KeyBeforeSection,
		EmptyKeyName,
		SectionNotFound,
		KeyNotFound,
		TypeMismatch,
	};

	class IniFile
	{
	public:
		using Value = std::variant<
			int32_t,
			float,
			fig::string,
			std::vector<int32_t>,
			std::vector<float>,
			std::vector<fig::string>
		>;

		void Set(const fig::string& section, const fig::string& key, int32_t value);
		void Set(const fig::string& section, const fig::string& key, float value);
		void Set(const fig::string& section, const fig::string& key, fig::string value);
		void Set(const fig::string& section, const fig::string& key, std::vector<int32_t> value);
		void Set(const fig::string& section, const fig::string& key, std::vector<float> value);
		void Set(const fig::string& section, const fig::string& key, std::vector<fig::string> value);

		template<typename T>
		[[nodiscard]] std::expected<T, IniError> Get(const fig::string& section, const fig::string& key) const
		{
			auto itSection = _sections.find(fig::string(section));
			if (itSection == _sections.end())
				return std::unexpected(IniError::SectionNotFound);

			auto itKey = itSection->second.values.find(fig::string(key));
			if (itKey == itSection->second.values.end())
				return std::unexpected(IniError::KeyNotFound);

			auto result = Coerce<T>(itKey->second);
			if (!result)
				return std::unexpected(IniError::TypeMismatch);

			return *result;
		}

		template<typename T>
		static std::optional<T> Coerce(const Value& val)
		{
			if (const T* exact = std::get_if<T>(&val))
				return *exact;

			else if constexpr (std::is_same_v<T, float>)
			{
				if (const auto* int_val = std::get_if<int32_t>(&val))
					return static_cast<float>(*int_val);
			}
			else if constexpr (std::is_same_v<T, int32_t>)
			{
				if (const auto* float_val = std::get_if<float>(&val))
					return static_cast<int32_t>(*float_val);
			}
			else if constexpr (std::is_same_v<T, std::string>)
			{
				if (const auto* int_val = std::get_if<int32_t>(&val))
					return int_to_string(*int_val);
				if (const auto* float_val = std::get_if<float>(&val))
					return float_to_string(*float_val);
			}

			return std::nullopt;
		}

		[[nodiscard]] bool HasKey(const fig::string& section, const fig::string& key) const;
		[[nodiscard]] bool HasSection(const fig::string& section) const;

		void Remove(const fig::string& section, const fig::string& key);
		void RemoveSection(const fig::string& section);
		void Clear();

		[[nodiscard]] std::expected<void, IniError> Load(const fig::path& path);
		[[nodiscard]] std::expected<void, IniError> Save(const fig::path& path) const;

	private:
		void SetValue(const fig::string& section, const fig::string& key, Value value);
		[[nodiscard]] fig::string Serialize() const;
		[[nodiscard]] std::expected<void, IniError> Deserialize(const fig::string& content);

	private:
		struct Section
		{
			std::vector<fig::string> key_order;
			std::map<fig::string, Value> values;
		};

		std::vector<fig::string> _section_order;
		std::map<fig::string, Section> _sections;
	};
}