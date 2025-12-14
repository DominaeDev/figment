#ifndef STRING_UTIL_H__
#define STRING_UTIL_H__

#include "Types.h"
#include <vector>
#include <unordered_set>

namespace fig::string_util
{
	void ltrim_str(string& s);
	void rtrim_str(string& s);

	inline void trim_str(string& s) { ltrim_str(s); rtrim_str(s); }
	[[nodiscard]] string ltrim(const string& s);
	[[nodiscard]] string rtrim(const string& s);
	[[nodiscard]] inline string trim(const string& s) { return ltrim(rtrim(s)); }

	[[nodiscard]] string lcase(const string& s);
	[[nodiscard]] string ucase(const string& s);
	[[nodiscard]] std::wstring lcase(const std::wstring& s);
	[[nodiscard]] std::wstring ucase(const std::wstring& s);
	int compare(const string& a, const string& b, bool ignore_case = false);
	bool equals(const string& a, const string& b, bool ignore_case = false);

	string& replace(string& str, const string& find, const string& replace);
	string& replace_all(string& str, const string& find, const string& replace);

	bool empty_or_whitespace(const string& s);
	bool begins_with(const string& str, const string& prefix, bool ignore_case = false);
	bool ends_with(const string& str, const string& suffix, bool ignore_case = false);
	std::vector<string> split(string s, char delimiter, bool removeEmpty = true);
	std::vector<string> split(const string& input, const std::unordered_set<char>& delimiters, bool removeEmpty);

	string& normalize_newlines(string& text);
	string normalize_newlines(string&& s);

	size_t validate_utf8(const string& text) noexcept;
	std::wstring from_utf8(const string& str);
	string to_utf8(const std::wstring& str);
}

#endif 