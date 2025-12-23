#include <pch.h>
#include "util/StringUtility.h"

namespace fig::string_util
{
	void ltrim_inplace(string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
	}

	string ltrim(const string& in)
	{
		string s(in);
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
		return s;
	}

	void rtrim_inplace(string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	string rtrim(const string& in)
	{
		string s(in);
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), s.end());
		return s;
	}

	bool empty_or_whitespace(const string& s) noexcept
	{
		static constexpr const_string ws { " \t\r\n\v\f" };
		return s.size() == 0 || s.find_first_not_of(ws.data(), 0, ws.length()) == fig::npos;
	}

	bool is_whitespace(char ch) noexcept
	{
		return std::isspace(static_cast<unsigned char>(ch));
	}

	bool is_punctuation(char ch) noexcept
	{
		return std::ispunct(static_cast<unsigned char>(ch));
	}

	string lcase(const string& str)
	{
		wstring s = from_utf8(str);
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towlower(c); });
		return to_utf8(s);
	}

	string ucase(const string& str)
	{
		wstring s = from_utf8(str);
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towupper(c); });
		return to_utf8(s);
	}

	wstring lcase(const wstring& str)
	{
		wstring s = str;
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towlower(c); });
		return s;
	}

	wstring ucase(const wstring& str)
	{
		wstring s = str;
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towupper(c); });
		return s;
	}

	int compare(const string& a, const string& b, bool ignore_case)
	{
		wstring wa = from_utf8(a);
		wstring wb = from_utf8(b);

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

	bool equals(const string& a, const string& b, bool ignore_case)
	{
		if (!ignore_case)
			return a == b;
		return compare(a, b, true) == 0;
	}

	bool begins_with(const string& str, const string& prefix, bool ignore_case)
	{
		wstring wstr = from_utf8(str);
		wstring wprefix = from_utf8(prefix);

		if (wstr.size() < wprefix.size())
			return false;
		const wstring begin_piece = wstr.substr(0, wprefix.length());

		if (ignore_case)
			return std::equal(begin_piece.begin(), begin_piece.end(), wprefix.begin(), wprefix.end(), [](wchar_t a, wchar_t b) {return std::towlower(a) == std::towlower(b); });
		else
			return begin_piece == wprefix;
	}

	bool ends_with(const string& str, const string& suffix, bool ignore_case)
	{
		wstring wstr = from_utf8(str);
		wstring wsuffix = from_utf8(suffix);

		if (wstr.size() < wsuffix.size())
			return false;

		const wstring end_piece = wstr.substr(wstr.length() - wsuffix.length());

		if (ignore_case)
			return std::equal(end_piece.begin(), end_piece.end(), suffix.begin(), suffix.end(), [](wchar_t a, wchar_t b) {return std::towlower(a) == std::towlower(b); });
		else
			return end_piece == wsuffix;
	}

	string replace(const string& str, const string& find, const string& replace)
	{
		auto&& pos = str.find(find);
		if (pos != fig::npos)
		{
			string copy = str;
			copy.replace(pos, find.length(), replace);
			return copy;
		}
		return str;
	}

	string replace_all(const string& str, const string& find, const string& replace)
	{
		auto&& pos = str.find(find);
		if (pos != fig::npos)
		{
			string copy = str;
			while (pos != fig::npos)
			{
				copy.replace(pos, find.length(), replace);
				pos = copy.find(find, pos + replace.length());
			}
			return copy;
		}
		return str;
	}

	string& replace_inplace(string& str, const string& find, const string& replace)
	{
		auto&& pos = str.find(find);
		if (pos != fig::npos)
			str.replace(pos, find.length(), replace);
		return str;
	}

	string& replace_all_inplace(string& str, const string& find, const string& replace)
	{
		auto&& pos = str.find(find);
		while (pos != fig::npos)
		{
			str.replace(pos, find.length(), replace);
			pos = str.find(find, pos + replace.length());
		}
		return str;
	}

	std::vector<string> split(string s, char delimiter, bool removeEmpty)
	{
		std::vector<string> sections;
		size_t pos = 0;
		while ((pos = s.find(delimiter)) != fig::npos)
		{
			string token = s.substr(0, pos);
			if (!removeEmpty || !empty_or_whitespace(token))
				sections.push_back(token);
			s.erase(0, pos + 1);
		}
		if (!removeEmpty || !empty_or_whitespace(s))
			sections.push_back(s); // Remainder

		std::transform(sections.begin(), sections.end(), sections.begin(), [](string str) {
			return trim(str);
		});

		return sections;
	}

	std::vector<string> split(const string& input, const std::unordered_set<char>& delimiters, bool removeEmpty)
	{
		std::vector<string> result;
		string token;

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

	string& normalize_newlines(string& text)
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

	string normalize_newlines(string&& text)
	{
		return normalize_newlines(text); // rvo
	}

	wstring& normalize_newlines(wstring& text)
	{
		size_t cursor_write = 0;

		for (size_t cursor_read = 0; cursor_read < text.size(); ++cursor_read)
		{
			if (text[cursor_read] == L'\r')
			{
				// Skip CR and optional following LF
				if (cursor_read + 1 < text.size() && text[cursor_read + 1] == L'\n')
					++cursor_read;
				text[cursor_write++] = L'\n';
			}
			else
				text[cursor_write++] = text[cursor_read];
		}

		text.resize(cursor_write);
		return text;
	}

	wstring normalize_newlines(wstring&& text)
	{
		return normalize_newlines(text); // rvo
	}

	wstring from_utf8(const string& str)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.from_bytes(str);
	}

	string to_utf8(const wstring& str)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(str);
	}

	size_t validate_utf8(const string& text) noexcept
	{
		size_t len = text.size();
		if (len == 0) return 0;

		// Check the last few bytes to see if a multi-byte character is cut off
		for (size_t i = 1; i <= 4 && i <= len; ++i)
		{
			unsigned char c = text[len - i];
			// Check for start of a multi-byte sequence from the end
			if ((c & 0xE0) == 0xC0)
			{
				// 2-byte character start: 110xxxxx
				// Needs at least 2 bytes
				if (i < 2) return len - i;
			}
			else if ((c & 0xF0) == 0xE0)
			{
				// 3-byte character start: 1110xxxx
				// Needs at least 3 bytes
				if (i < 3) return len - i;
			}
			else if ((c & 0xF8) == 0xF0)
			{
				// 4-byte character start: 11110xxx
				// Needs at least 4 bytes
				if (i < 4) return len - i;
			}
		}

		// If no cut-off multi-byte character is found, return full length
		return len;
	}

	int32_t count_words(const string& text, int32_t cursor, int32_t& pos_last_word)
	{
		if (text.empty())
		{
			pos_last_word = 0;
			return 0;
		}

		if (cursor < 0 || cursor > toI(text.length()))
			cursor = toI(text.length());

		pos_last_word = 0;
		int32_t count = 1;

		auto fnFindWhitespace = [](const char& ch) -> bool { return fig::string_util::is_whitespace(ch); };
		auto fnFindNotWhitespace = [](const char& ch) -> bool { return !fig::string_util::is_whitespace(ch); };

		int32_t pos = fig::string_util::find_index(text, 0, fnFindWhitespace);
		while (pos != -1 && pos < cursor)
		{
			int next_word = fig::string_util::find_index(text, pos, fnFindNotWhitespace);
			if (next_word == -1 || next_word >= cursor)
				return count;
			count++;
			pos_last_word = next_word;
			pos = fig::string_util::find_index(text, next_word, fnFindWhitespace);
		}
		return count;
	};
}