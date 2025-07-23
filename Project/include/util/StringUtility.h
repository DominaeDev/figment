#pragma once

#include <string>
#include <vector>

struct string_util
{
	static void ltrim_str(std::string& s);
	static std::string ltrim(const std::string& s);
	static void rtrim_str(std::string& s);
	static std::string rtrim(const std::string& s);
	static inline std::string trim(const std::string& s) { return ltrim(rtrim(s)); }
	static inline void trim_str(std::string& s) { ltrim_str(s); rtrim_str(s); }

	static std::string lcase(const std::string& s);
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

	static std::wstring from_utf8(const std::string& str);
	static std::string to_utf8(const std::wstring& str);

	string_util() = delete;
};