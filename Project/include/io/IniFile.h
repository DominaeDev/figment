#pragma once

#include "Figment.h"

#include <variant>
#include <map>

namespace fig::io
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
			fig::fixed,
			fig::string,
			std::vector<fig::fixed>,
			std::vector<fig::string>
		>;

		void Set(const fig::string_view section, const fig::string_view key, fig::fixed value);
		void Set(const fig::string_view section, const fig::string_view key, fig::string value);
		void Set(const fig::string_view section, const fig::string_view key, std::vector<fig::fixed> value);
		void Set(const fig::string_view section, const fig::string_view key, std::vector<fig::string> value);

		template<typename T>
		[[nodiscard]] std::expected<T, IniError> Get(fig::string_view section, fig::string_view key) const
		{
			auto itSection = _sections.find(fig::string_view(section));
			if (itSection == _sections.end())
				return std::unexpected(IniError::SectionNotFound);

			auto itKey = itSection->second.values.find(fig::string_view(key));
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
			if constexpr (std::is_same_v<T, bool>)
			{
				if (const auto* str_val = std::get_if<fig::string>(&val))
					return string_to_bool(*str_val);
			}
			else
			{
				if (const T* exact = std::get_if<T>(&val))
					return *exact;
			}

			if constexpr (std::is_same_v<T, std::string>)
			{
				if (const auto* fixed_val = std::get_if<fig::fixed>(&val))
					return fixed_to_string(*fixed_val);
			}

			return std::nullopt;
		}

		[[nodiscard]] bool HasKey(const fig::string_view section, fig::string_view key) const;
		[[nodiscard]] bool HasSection(const fig::string_view section) const;

		void Remove(fig::string_view section, fig::string_view key);
		void RemoveSection(fig::string_view section);
		void Clear();

		[[nodiscard]] std::expected<void, IniError> Load(const fig::path& path);
		[[nodiscard]] std::expected<void, IniError> Save(const fig::path& path) const;

	private:
		void SetValue(const fig::string& section, const fig::string& key, Value value)
		{
			SetValue(fig::string_view { section }, fig::string_view { key }, value);
		}
		void SetValue(const fig::string_view section, const fig::string_view key, Value value);
		[[nodiscard]] fig::string Serialize() const;
		[[nodiscard]] std::expected<void, IniError> Deserialize(const fig::string& content);

	private:
		struct Section
		{
			std::vector<fig::string> key_order;
			std::map<fig::string, Value, std::less<>> values;
		};

		std::vector<fig::string> _section_order;
		std::map<fig::string, Section, std::less<>> _sections;
	};
}