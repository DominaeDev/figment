#ifndef STRING_UTIL_H__
#define STRING_UTIL_H__

#include <string>
#include <vector>
#include <unordered_set>

namespace string_util
{
	void ltrim_str(std::string& s);
	[[nodiscard]] std::string ltrim(const std::string& s);
	void rtrim_str(std::string& s);
	[[nodiscard]] std::string rtrim(const std::string& s);
	[[nodiscard]] inline std::string trim(const std::string& s) { return ltrim(rtrim(s)); }
	inline void trim_str(std::string& s) { ltrim_str(s); rtrim_str(s); }

	[[nodiscard]] std::string lcase(const std::string& s);
	[[nodiscard]] std::string ucase(const std::string& s);
	[[nodiscard]] std::wstring lcase(const std::wstring& s);
	[[nodiscard]] std::wstring ucase(const std::wstring& s);
	int compare(const std::string& a, const std::string& b, bool ignore_case = false);
	bool equals(const std::string& a, const std::string& b, bool ignore_case = false);

	std::string& replace(std::string& str, const std::string& find, const std::string& replace);
	std::string& replace_all(std::string& str, const std::string& find, const std::string& replace);
	std::string get_filename(const std::string& str);

	bool empty_or_whitespace(const std::string& s);
	bool begins_with(const std::string& str, const std::string& prefix, bool ignore_case = false);
	bool ends_with(const std::string& str, const std::string& suffix, bool ignore_case = false);
	std::vector<std::string> split(std::string s, char delimiter, bool removeEmpty = true);
	std::vector<std::string> split(const std::string& input, const std::unordered_set<char>& delimiters, bool removeEmpty);

	std::string& normalize_newlines(std::string& text);
	std::string normalize_newlines(std::string&& s);

	size_t validate_utf8(const std::string& text) noexcept;
	std::wstring from_utf8(const std::string& str);
	std::string to_utf8(const std::wstring& str);
}

#endif 