#ifndef STRING_UTIL_H__
#define STRING_UTIL_H__

#include "Types.h"
#include <vector>
#include <unordered_set>

namespace string_util
{
	void ltrim_str(fig::string& s);
	void rtrim_str(fig::string& s);

	inline void trim_str(fig::string& s) { ltrim_str(s); rtrim_str(s); }
	[[nodiscard]] fig::string ltrim(const fig::string& s);
	[[nodiscard]] fig::string rtrim(const fig::string& s);
	[[nodiscard]] inline fig::string trim(const fig::string& s) { return ltrim(rtrim(s)); }

	[[nodiscard]] fig::string lcase(const fig::string& s);
	[[nodiscard]] fig::string ucase(const fig::string& s);
	[[nodiscard]] std::wstring lcase(const std::wstring& s);
	[[nodiscard]] std::wstring ucase(const std::wstring& s);
	int compare(const fig::string& a, const fig::string& b, bool ignore_case = false);
	bool equals(const fig::string& a, const fig::string& b, bool ignore_case = false);

	fig::string& replace(fig::string& str, const fig::string& find, const fig::string& replace);
	fig::string& replace_all(fig::string& str, const fig::string& find, const fig::string& replace);

	bool empty_or_whitespace(const fig::string& s);
	bool begins_with(const fig::string& str, const fig::string& prefix, bool ignore_case = false);
	bool ends_with(const fig::string& str, const fig::string& suffix, bool ignore_case = false);
	std::vector<fig::string> split(fig::string s, char delimiter, bool removeEmpty = true);
	std::vector<fig::string> split(const fig::string& input, const std::unordered_set<char>& delimiters, bool removeEmpty);

	fig::string& normalize_newlines(fig::string& text);
	fig::string normalize_newlines(fig::string&& s);

	size_t validate_utf8(const fig::string& text) noexcept;
	std::wstring from_utf8(const fig::string& str);
	fig::string to_utf8(const std::wstring& str);
}

#endif 