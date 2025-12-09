export module Utility:StringUtil;

import std;

export namespace string_util
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

	std::wstring from_utf8(const std::string& str);
	std::string to_utf8(const std::wstring& str);
}

void string_util::ltrim_str(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

std::string string_util::ltrim(const std::string& in)
{
	std::string s(in);
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	return s;
}

void string_util::rtrim_str(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

std::string string_util::rtrim(const std::string& in)
{
	std::string s(in);
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
	return s;
}

bool string_util::empty_or_whitespace(const std::string& s)
{
	return s.size() == 0 || s.find_first_not_of(" \t\r\n", 0, 4) == std::string::npos;
}

std::string string_util::get_filename(const std::string& str)
{
	size_t pos = str.find_last_of("\\/");
	if (pos == std::string::npos)
		return str;
	return str.substr(pos + 1);
}

std::string string_util::lcase(const std::string& str)
{
	std::wstring s = from_utf8(str);
	std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towlower(c); });
	return to_utf8(s);
}

std::string string_util::ucase(const std::string& str)
{
	std::wstring s = from_utf8(str);
	std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towupper(c); });
	return to_utf8(s);
}

std::wstring string_util::lcase(const std::wstring& str)
{
	std::wstring s = str;
	std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towlower(c); });
	return s;
}

std::wstring string_util::ucase(const std::wstring& str)
{
	std::wstring s = str;
	std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towupper(c); });
	return s;
}

int string_util::compare(const std::string& a, const std::string& b, bool ignore_case)
{
	std::wstring wa = from_utf8(a);
	std::wstring wb = from_utf8(b);

	if (ignore_case)
	{
		wa = lcase(wa);
		wb = lcase(wb);
	}
	if (wa < wb)
		return -1;
	if (wa > wb)
		return 1;
	return 0;
}

bool string_util::equals(const std::string& a, const std::string& b, bool ignore_case)
{
	if (!ignore_case)
		return a == b;
	return compare(a, b, true) == 0;
}

bool string_util::begins_with(const std::string& str, const std::string& prefix, bool ignore_case)
{
	std::wstring wstr = from_utf8(str);
	std::wstring wprefix = from_utf8(prefix);

	if (wstr.size() < wprefix.size())
		return false;
	const std::wstring begin_piece = wstr.substr(0, wprefix.length());

	if (ignore_case)
		return std::equal(begin_piece.begin(), begin_piece.end(), wprefix.begin(), wprefix.end(), [](wchar_t a, wchar_t b) {return std::towlower(a) == std::towlower(b); });
	else
		return begin_piece == wprefix;
}

bool string_util::ends_with(const std::string& str, const std::string& suffix, bool ignore_case)
{
	std::wstring wstr = from_utf8(str);
	std::wstring wsuffix = from_utf8(suffix);

	if (wstr.size() < wsuffix.size())
		return false;

	const std::wstring end_piece = wstr.substr(wstr.length() - wsuffix.length());

	if (ignore_case)
		return std::equal(end_piece.begin(), end_piece.end(), suffix.begin(), suffix.end(), [](wchar_t a, wchar_t b) {return std::towlower(a) == std::towlower(b); });
	else
		return end_piece == wsuffix;
}

std::string& string_util::replace(std::string& str, const std::string& find, const std::string& replace)
{
	auto&& pos = str.find(find);
	if (pos != std::string::npos)
		str.replace(pos, find.length(), replace);
	return str;
}

std::string& string_util::replace_all(std::string& str, const std::string& find, const std::string& replace)
{
	auto&& pos = str.find(find);
	while (pos != std::string::npos)
	{
		str.replace(pos, find.length(), replace);
		pos = str.find(find, pos + replace.length());
	}
	return str;
}

std::vector<std::string> string_util::split(std::string s, char delimiter, bool removeEmpty)
{
	std::vector<std::string> sections;
	size_t pos = 0;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		std::string token = s.substr(0, pos);
		if (!removeEmpty || !empty_or_whitespace(token))
			sections.push_back(token);
		s.erase(0, pos + 1);
	}
	if (!removeEmpty || !empty_or_whitespace(s))
		sections.push_back(s); // Remainder

	std::transform(sections.begin(), sections.end(), sections.begin(), [](std::string str) {
		return string_util::trim(str);
	});

	return sections;
}

std::vector<std::string> string_util::split(const std::string& input, const std::unordered_set<char>& delimiters, bool removeEmpty)
{
	std::vector<std::string> result;
	std::string token;

	for (char ch : input)
	{
		if (delimiters.find(ch) != delimiters.end())
		{
			if (!token.empty())
			{
				result.push_back(token);
				token.clear();
			}
		}
		else
			token += ch;
	}

	if (!removeEmpty || !token.empty())
		result.push_back(token);

	return result;
}

std::string& string_util::normalize_newlines(std::string& text)
{
	size_t cursor_write = 0;

	for (size_t cursor_read = 0; cursor_read < text.size(); ++cursor_read)
	{
		if (text[cursor_read] == '\r')
		{
			// Skip CR and optional following LF
			if (cursor_read + 1 < text.size() && text[cursor_read + 1] == '\n')
				++cursor_read;
			text[cursor_write++] = '\n';
		}
		else
			text[cursor_write++] = text[cursor_read];
	}

	text.resize(cursor_write);
	return text;
}

std::string string_util::normalize_newlines(std::string&& text)
{
	return normalize_newlines(text); // rvo
}

std::wstring string_util::from_utf8(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(str);
}

std::string string_util::to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(str);
}