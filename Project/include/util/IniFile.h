#pragma once

#include "Types.h"

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
		KeyBeforeGroup,
		EmptyKeyName,
		GroupNotFound,
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
			auto itGroup = _sections.find(fig::string(section));
			if (itGroup == _sections.end())
				return std::unexpected(IniError::GroupNotFound);

			auto itKey = itGroup->second.values.find(fig::string(key));
			if (itKey == itGroup->second.values.end())
				return std::unexpected(IniError::KeyNotFound);

			auto result = Coerce<T>(itKey->second);
			if (!result)
				return std::unexpected(IniError::TypeMismatch);

			return *result;
		}

		[[nodiscard]] bool HasKey(const fig::string& section, const fig::string& key) const;
		[[nodiscard]] bool HasGroup(const fig::string& section) const;

		void Remove(const fig::string& section, const fig::string& key);
		void RemoveGroup(const fig::string& section);
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