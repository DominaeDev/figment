#pragma once

#include "Figment.h"
#include <unordered_set>

namespace fig
{
	[[nodiscard]] fig::string ltrim(const fig::string& s);
	[[nodiscard]] fig::string rtrim(const fig::string& s);
	[[nodiscard]] inline fig::string trim(const fig::string& s) { return ltrim(rtrim(s)); }
	void ltrim_inplace(fig::string& s);
	void rtrim_inplace(fig::string& s);
	inline void trim_inplace(fig::string& s) { ltrim_inplace(s); rtrim_inplace(s); }
	fig::string trim(fig::string&& s);
	fig::string_view ltrim(fig::string_view s);
	fig::string_view rtrim(fig::string_view s);
	fig::string_view trim(fig::string_view s);
	fig::string_view trunc(fig::string_view s, size_t length);

	[[nodiscard]] fig::wstring ltrim(const fig::wstring& s);
	[[nodiscard]] fig::wstring rtrim(const fig::wstring& s);
	[[nodiscard]] inline fig::wstring trim(const fig::wstring& s) { return ltrim(rtrim(s)); }
	void ltrim_inplace(fig::wstring& s);
	void rtrim_inplace(fig::wstring& s);
	inline void trim_inplace(fig::wstring& s) { ltrim_inplace(s); rtrim_inplace(s); }
	fig::wstring trim(fig::wstring&& s);

	[[nodiscard]] fig::string lcase(const fig::string& s);
	[[nodiscard]] fig::string ucase(const fig::string& s);
	fig::string& lcase_inplace(fig::string& s);
	fig::string& ucase_inplace(fig::string& s);
	[[nodiscard]] fig::wstring lcase(const fig::wstring& s);
	[[nodiscard]] fig::wstring ucase(const fig::wstring& s);
	fig::wstring& lcase_inplace(fig::wstring& s);
	fig::wstring& ucase_inplace(fig::wstring& s);

	fig::string unindent(const fig::string& s);
	void unindent_inplace(fig::string& s);

	fig::string escape(const fig::string& s) noexcept;
	void escape_inplace(fig::string& s) noexcept;
	fig::string unescape(const fig::string& s) noexcept;
	void unescape_inplace(fig::string& s) noexcept;

	[[nodiscard]] int compare(const fig::string& a, const fig::string& b, bool ignore_case = false);
	[[nodiscard]] bool equals(const fig::string& a, const fig::string& b, bool ignore_case = false);
	[[nodiscard]] bool equals(const string_view& a, const string_view& b, bool ignore_case = false);

	fig::string replace(const fig::string& str, const fig::string& find, const fig::string& replace);
	fig::string replace_all(const fig::string& str, const fig::string& find, const fig::string& replace);

	fig::string& replace_inplace(fig::string& str, const fig::string& find, const fig::string& replace);
	fig::string& replace_all_inplace(fig::string& str, const fig::string& find, const fig::string& replace);

	bool is_whitespace(char ch) noexcept;
	bool is_punctuation(char ch) noexcept;
	bool empty_or_whitespace(const fig::string& s) noexcept;
	bool empty_or_whitespace(const fig::wstring& s) noexcept;
	bool begins_with(const fig::string& str, const fig::string& prefix, bool ignore_case = false);
	bool ends_with(const fig::string& str, const fig::string& suffix, bool ignore_case = false);
	std::vector<fig::string> split(fig::string s, char delimiter, bool removeEmpty = true);
	std::vector<fig::string> split(const fig::string& input, const std::unordered_set<char>& delimiters, bool removeEmpty);

	fig::string& normalize_newlines(fig::string& text);
	[[nodiscard]] fig::string normalize_newlines(fig::string&& s);

	fig::wstring& normalize_newlines(fig::wstring& text);
	[[nodiscard]] fig::wstring normalize_newlines(fig::wstring&& s);

	fig::string bool_to_string(bool value);
	fig::string int_to_string(int32_t value);
	fig::string float_to_string(float value);
	fig::string fixed_to_string(fig::fixed value);
	bool string_to_bool(const fig::string_view& s, bool default_value);
	int32_t string_to_int(const fig::string_view& s, int32_t default_value);
	float string_to_float(const fig::string_view& s, float default_value);
	fig::fixed string_to_fixed(const fig::string_view& s, fig::fixed default_value);
	std::optional<bool> string_to_bool(const fig::string_view& s);
	std::optional<int32_t> string_to_int(const fig::string_view& s);
	std::optional<float> string_to_float(const fig::string_view& s);
	std::optional<fig::fixed> string_to_fixed(const fig::string_view& s);

	size_t validate_utf8(const fig::string& text) noexcept;
	fig::wstring from_utf8(const fig::string& str);
	fig::string to_utf8(const fig::wstring& str);

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
