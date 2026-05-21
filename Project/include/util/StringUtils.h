#ifndef STRING_UTIL_H__
#define STRING_UTIL_H__

#include "Figment.h"
#include <unordered_set>

namespace fig
{
	[[nodiscard]] string ltrim(const string& s);
	[[nodiscard]] string rtrim(const string& s);
	[[nodiscard]] inline string trim(const string& s) { return ltrim(rtrim(s)); }
	void ltrim_inplace(string& s);
	void rtrim_inplace(string& s);
	inline void trim_inplace(string& s) { ltrim_inplace(s); rtrim_inplace(s); }
	string trim(string&& s);

	[[nodiscard]] string lcase(const string& s);
	[[nodiscard]] string ucase(const string& s);
	string& lcase_inplace(string& s);
	string& ucase_inplace(string& s);
	[[nodiscard]] wstring lcase(const wstring& s);
	[[nodiscard]] wstring ucase(const wstring& s);
	wstring& lcase_inplace(wstring& s);
	wstring& ucase_inplace(wstring& s);

	[[nodiscard]] int compare(const string& a, const string& b, bool ignore_case = false);
	[[nodiscard]] bool equals(const string& a, const string& b, bool ignore_case = false);
	[[nodiscard]] bool equals(const string_view& a, const string_view& b, bool ignore_case = false);

	string replace(const string& str, const string& find, const string& replace);
	string replace_all(const string& str, const string& find, const string& replace);

	string& replace_inplace(string& str, const string& find, const string& replace);
	string& replace_all_inplace(string& str, const string& find, const string& replace);

	bool is_whitespace(char ch) noexcept;
	bool is_punctuation(char ch) noexcept;
	bool empty_or_whitespace(const string& s) noexcept;
	bool empty_or_whitespace(const wstring& s) noexcept;
	bool begins_with(const string& str, const string& prefix, bool ignore_case = false);
	bool ends_with(const string& str, const string& suffix, bool ignore_case = false);
	std::vector<string> split(string s, char delimiter, bool removeEmpty = true);
	std::vector<string> split(const string& input, const std::unordered_set<char>& delimiters, bool removeEmpty);

	string& normalize_newlines(string& text);
	[[nodiscard]] string normalize_newlines(string&& s);

	wstring& normalize_newlines(wstring& text);
	[[nodiscard]] wstring normalize_newlines(wstring&& s);

	fig::string int_to_string(int32_t value);
	fig::string float_to_string(float value);

	size_t validate_utf8(const string& text) noexcept;
	wstring from_utf8(const string& str);
	string to_utf8(const wstring& str);

	template<typename Str, typename Pred>
	size_t find_index(const Str& str, int32_t pos, Pred pred)
	{
		auto it_pos = str.cbegin();
		std::advance(it_pos, pos);
		if (it_pos < str.cbegin() || it_pos >= str.cend())
			return fig::npos;

		auto it = std::find_if(it_pos, str.cend(), pred);
		if (it != str.cend())
			return toI(std::distance(str.cbegin(), it));
		return fig::npos;
	};

	template<typename Str, typename V = Str::value_type>
	size_t index_of(const Str& str, int32_t pos, V value)
	{
		auto it_pos = str.cbegin();
		std::advance(it_pos, pos);
		if (it_pos < str.cbegin() || it_pos >= str.cend())
			return fig::npos;

		auto it = std::find(it_pos, str.cend(), value);
		if (it != str.cend())
			return toI(std::distance(str.cbegin(), it));
		return fig::npos;
	};

	[[nodiscard]] bool find_in(const std::string_view substr, const std::string_view text, bool case_insensitive = false, bool whole_words = false);
	[[nodiscard]] bool find_in(const std::wstring_view substr, const std::wstring_view text, bool case_insensitive = false, bool whole_words = false);

	[[nodiscard]] fig::string encode_csv(fig::string_span values);
	[[nodiscard]] fig::string_list decode_csv(const fig::string& csv);

	[[nodiscard]] fig::wstring strip_diacritics(fig::wstring&& input);
	[[nodiscard]] fig::string strip_diacritics(fig::string&& input);
}

#endif 