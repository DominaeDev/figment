#pragma once

#include <string>
#include <vector>

struct string_util
{
	static std::string& ltrim(std::string& s);
	static std::string ltrim(const std::string& s);
	static std::string& rtrim(std::string& s);
	static std::string rtrim(const std::string& s);
	static inline std::string& trim(std::string& s) { return ltrim(rtrim(s)); }
	static inline std::string trim(const std::string& s) { return ltrim(rtrim(s)); }

	static std::string& lcase(std::string& str);
	static std::string lcase(const std::string& s);
	static std::string& ucase(std::string& str);
	static std::string ucase(const std::string& s);

	static std::string& replace(std::string& str, const std::string& find, const std::string& replace);
	static std::string& replace_all(std::string& str, const std::string& find, const std::string& replace);
	static std::string get_filename(const std::string& str);

	static bool empty_or_whitespace(const std::string& s);
	static bool begins_with(const std::string_view& str, const std::string_view& prefix);
	static bool ends_with(const std::string_view& str, const std::string_view& suffix);
	static std::vector<std::string> split(std::string s, const std::string& delimiter);

	static std::string& normalize_newlines(std::string& text);
	static std::string normalize_newlines(std::string&& s);

	string_util() = delete;
};