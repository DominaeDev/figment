#include <pch.h>
#include <codecvt>
#include <cwctype>

namespace fig
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

	string trim(string&& in)
	{
		string s(std::move(in));
		trim_inplace(s);
		return s;
	}

	void ltrim_inplace(wstring& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wint_t ch) {
			return !std::iswspace(ch); 
		}));
	}

	wstring ltrim(const wstring& in)
	{
		wstring s(in);
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wint_t ch) {
			return !std::iswspace(ch); 
		}));
		return s;
	}

	void rtrim_inplace(wstring& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](wint_t ch) {
			return !std::iswspace(ch);
		}).base(), s.end());
	}

	wstring rtrim(const wstring& in)
	{
		wstring s(in);
		s.erase(std::find_if(s.rbegin(), s.rend(), [](wint_t ch) {
			return !std::iswspace(ch);
		}).base(), s.end());
		return s;
	}

	wstring trim(wstring&& in)
	{
		wstring s(std::move(in));
		trim_inplace(s);
		return s;
	}

	bool empty_or_whitespace(const string& s) noexcept
	{
		static constexpr const_string ws { " \t\r\n\v\f" };
		return s.size() == 0 || s.find_first_not_of(ws.data(), 0, ws.length()) == fig::npos;
	}

	bool empty_or_whitespace(const wstring& s) noexcept
	{
		static constexpr std::wstring_view ws { L" \t\r\n\v\f" };
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

	string& lcase_inplace(string& str)
	{
		wstring s = from_utf8(str);
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towlower(c); });
		str = to_utf8(s);
		return str;
	}

	string& ucase_inplace(string& str)
	{
		wstring s = from_utf8(str);
		std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return std::towupper(c); });
		str = to_utf8(s);
		return str;
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

	wstring& lcase_inplace(wstring& str)
	{
		for (size_t i = 0; i < str.length(); ++i)
			str[i] = static_cast<wchar_t>(std::towlower(static_cast<wint_t>(str[i])));
		return str;
	}

	wstring& ucase_inplace(wstring& str)
	{
		for (size_t i = 0; i < str.length(); ++i)
			str[i] = static_cast<wchar_t>(std::towupper(static_cast<wint_t>(str[i])));
		return str;
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

	bool equals(const string_view& a, const string_view& b, bool ignore_case)
	{
		if (a.size() != b.size())
			return false;

		return std::ranges::equal(a, b, [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
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
	
	bool find_in(const std::string_view substr, const std::string_view text, bool case_insensitive, bool whole_words)
	{
		std::wstring substrUtf8 = from_utf8(fig::string(substr));
		std::wstring textUtf8 = from_utf8(fig::string(text));
		return find_in(substrUtf8, textUtf8, case_insensitive, whole_words);
	}

	bool find_in(const std::wstring_view substr, const std::wstring_view text, bool case_insensitive, bool whole_words)
	{
		if (substr.empty() or text.size() < substr.size())
			return false;

		const auto char_equal = [&](wchar_t a, wchar_t b) -> bool
		{
			if (case_insensitive)
				return std::towlower(a) == std::towlower(b);
			return a == b;
		};

		const auto is_word_char = [](wchar_t c) -> bool
		{
			return std::iswalnum(c) || c == L'_' || c == '\'';
		};

		auto it = text.cbegin();

		while (it != text.cend())
		{
			auto found = std::ranges::search(std::ranges::subrange(it, text.cend()), substr, char_equal);

			if (found.empty())
				return false;

			if (!whole_words)
				return true;

			const std::size_t pos = static_cast<std::size_t>(found.begin() - text.cbegin());
			const std::size_t len = substr.size();

			const bool left_ok = (pos == 0) || !is_word_char(text[pos - 1]);
			const bool right_ok = (pos + len >= text.size()) || !is_word_char(text[pos + len]);

			if (left_ok and right_ok)
				return true;

			it = found.begin() + 1;
		}

		return false;
	}

	fig::string encode_csv(fig::string_span values)
	{
		auto encode_value = [](const fig::string& value) {
			if (not (value.contains(',') or value.contains('"')))
				return value;

			// Quote value, escape quotes
			fig::string quoted;
			quoted.reserve(value.size() + 2);
			quoted += '"';
			for (const char c : value)
			{
				if (c == '"')
					quoted += "\"\"";
				else
					quoted += c;
			}
			quoted += '"';
			return quoted;
		};

		fig::string result;
		for (const auto& field : values)
		{
			if (!result.empty())
				result += ',';
			result += trim(encode_value(field));
		}
		return result;
	}

	fig::string_list decode_csv(const fig::string& csv)
	{
		fig::string_list fields;
		fig::string current;
		bool in_quotes = false;

		for (size_t i = 0; i < csv.size(); ++i)
		{
			const char ch = csv[i];

			if (in_quotes)
			{
				if (ch == '"' && i + 1 < csv.size() && csv[i + 1] == '"')
				{
					// Unescape
					current += '"';
					++i;
				}
				else if (ch == '"')
					in_quotes = false;
				else
					current += ch;
			}
			else
			{
				if (ch == '"') 
					in_quotes = true;
				else if (ch == ',')
				{
					fields.emplace_back(trim(std::move(current)));
					current.clear();
				}
				else 
					current += ch;
			}
		}

		if (not current.empty())
			fields.emplace_back(trim(std::move(current)));
		return fields;
	}

	fig::wstring strip_diacritics(fig::wstring&& input)
	{
		static const std::unordered_map<wchar_t, wchar_t> diacritic_map {
			// a
			{ L'\u00E0', L'a' }, { L'\u00E1', L'a' }, { L'\u00E2', L'a' }, { L'\u00E3', L'a' },
			{ L'\u00E4', L'a' }, { L'\u00E5', L'a' }, { L'\u0101', L'a' }, { L'\u0103', L'a' },
			{ L'\u0105', L'a' }, { L'\u01CE', L'a' }, { L'\u01DF', L'a' }, { L'\u01E1', L'a' },
			{ L'\u01FB', L'a' }, { L'\u0201', L'a' }, { L'\u0203', L'a' }, { L'\u0227', L'a' },
			{ L'\u00E6', L'a' },
			// c
			{ L'\u00E7', L'c' }, { L'\u0107', L'c' }, { L'\u0109', L'c' }, { L'\u010D', L'c' },
			{ L'\u010B', L'c' },
			// d
			{ L'\u010F', L'd' }, { L'\u0111', L'd' },
			// e
			{ L'\u00E8', L'e' }, { L'\u00E9', L'e' }, { L'\u00EA', L'e' }, { L'\u00EB', L'e' },
			{ L'\u0113', L'e' }, { L'\u0115', L'e' }, { L'\u0117', L'e' }, { L'\u0119', L'e' },
			{ L'\u011B', L'e' }, { L'\u0205', L'e' }, { L'\u0207', L'e' }, { L'\u0229', L'e' },
			// g
			{ L'\u011D', L'g' }, { L'\u011F', L'g' }, { L'\u0121', L'g' }, { L'\u0123', L'g' },
			{ L'\u01E7', L'g' }, { L'\u01F5', L'g' },
			// h
			{ L'\u0125', L'h' }, { L'\u0127', L'h' },
			// i
			{ L'\u00EC', L'i' }, { L'\u00ED', L'i' }, { L'\u00EE', L'i' }, { L'\u00EF', L'i' },
			{ L'\u0129', L'i' }, { L'\u012B', L'i' }, { L'\u012D', L'i' }, { L'\u012F', L'i' },
			{ L'\u0131', L'i' }, { L'\u01D0', L'i' }, { L'\u0209', L'i' }, { L'\u020B', L'i' },
			// j
			{ L'\u0135', L'j' },
			// k
			{ L'\u0137', L'k' }, { L'\u01E9', L'k' },
			// l
			{ L'\u013A', L'l' }, { L'\u013C', L'l' }, { L'\u013E', L'l' }, { L'\u0140', L'l' },
			{ L'\u0142', L'l' },
			// n
			{ L'\u00F1', L'n' }, { L'\u0144', L'n' }, { L'\u0146', L'n' }, { L'\u0148', L'n' },
			{ L'\u0149', L'n' }, { L'\u01F9', L'n' },
			// o
			{ L'\u00F2', L'o' }, { L'\u00F3', L'o' }, { L'\u00F4', L'o' }, { L'\u00F5', L'o' },
			{ L'\u00F6', L'o' }, { L'\u00F8', L'o' }, { L'\u014D', L'o' }, { L'\u014F', L'o' },
			{ L'\u0151', L'o' }, { L'\u01D2', L'o' }, { L'\u01EB', L'o' }, { L'\u01ED', L'o' },
			{ L'\u020D', L'o' }, { L'\u020F', L'o' }, { L'\u022B', L'o' }, { L'\u022D', L'o' },
			{ L'\u022F', L'o' }, { L'\u0231', L'o' },
			// r
			{ L'\u0155', L'r' }, { L'\u0157', L'r' }, { L'\u0159', L'r' }, { L'\u0211', L'r' },
			{ L'\u0213', L'r' },
			// s
			{ L'\u015B', L's' }, { L'\u015D', L's' }, { L'\u015F', L's' }, { L'\u0161', L's' },
			{ L'\u0219', L's' }, { L'\u00DF', L's' }, // eszett -> 's' (lossy but conventional)
			// t
			{ L'\u0163', L't' }, { L'\u0165', L't' }, { L'\u0167', L't' }, { L'\u021B', L't' },
			// u
			{ L'\u00F9', L'u' }, { L'\u00FA', L'u' }, { L'\u00FB', L'u' }, { L'\u00FC', L'u' },
			{ L'\u0169', L'u' }, { L'\u016B', L'u' }, { L'\u016D', L'u' }, { L'\u016F', L'u' },
			{ L'\u0171', L'u' }, { L'\u0173', L'u' }, { L'\u01D4', L'u' }, { L'\u01D6', L'u' },
			{ L'\u01D8', L'u' }, { L'\u01DA', L'u' }, { L'\u01DC', L'u' }, { L'\u0215', L'u' },
			{ L'\u0217', L'u' },
			// w
			{ L'\u0175', L'w' },
			// y
			{ L'\u00FD', L'y' }, { L'\u00FF', L'y' }, { L'\u0177', L'y' }, { L'\u0233', L'y' },
			// z
			{ L'\u017A', L'z' }, { L'\u017C', L'z' }, { L'\u017E', L'z' },
		};

		std::ranges::transform(input, input.begin(),
			[](auto& ch) -> wchar_t {
				if (const auto it = diacritic_map.find(ch); it != diacritic_map.end())
					return it->second;
				return ch;
			});

		return input;
	}

	fig::string strip_diacritics(fig::string&& input)
	{
		static const std::unordered_map<uint32_t, char> diacritic_map {
			// a
			{ 0x00E0, 'a' }, { 0x00E1, 'a' }, { 0x00E2, 'a' }, { 0x00E3, 'a' },
			{ 0x00E4, 'a' }, { 0x00E5, 'a' }, { 0x0101, 'a' }, { 0x0103, 'a' },
			{ 0x0105, 'a' }, { 0x01CE, 'a' }, { 0x01DF, 'a' }, { 0x01E1, 'a' },
			{ 0x01FB, 'a' }, { 0x0201, 'a' }, { 0x0203, 'a' }, { 0x0227, 'a' },
			{ 0x00E6, 'a' },
			// c
			{ 0x00E7, 'c' }, { 0x0107, 'c' }, { 0x0109, 'c' }, { 0x010D, 'c' },
			{ 0x010B, 'c' },
			// d
			{ 0x010F, 'd' }, { 0x0111, 'd' },
			// e
			{ 0x00E8, 'e' }, { 0x00E9, 'e' }, { 0x00EA, 'e' }, { 0x00EB, 'e' },
			{ 0x0113, 'e' }, { 0x0115, 'e' }, { 0x0117, 'e' }, { 0x0119, 'e' },
			{ 0x011B, 'e' }, { 0x0205, 'e' }, { 0x0207, 'e' }, { 0x0229, 'e' },
			// g
			{ 0x011D, 'g' }, { 0x011F, 'g' }, { 0x0121, 'g' }, { 0x0123, 'g' },
			{ 0x01E7, 'g' }, { 0x01F5, 'g' },
			// h
			{ 0x0125, 'h' }, { 0x0127, 'h' },
			// i
			{ 0x00EC, 'i' }, { 0x00ED, 'i' }, { 0x00EE, 'i' }, { 0x00EF, 'i' },
			{ 0x0129, 'i' }, { 0x012B, 'i' }, { 0x012D, 'i' }, { 0x012F, 'i' },
			{ 0x0131, 'i' }, { 0x01D0, 'i' }, { 0x0209, 'i' }, { 0x020B, 'i' },
			// j
			{ 0x0135, 'j' },
			// k
			{ 0x0137, 'k' }, { 0x01E9, 'k' },
			// l
			{ 0x013A, 'l' }, { 0x013C, 'l' }, { 0x013E, 'l' }, { 0x0140, 'l' },
			{ 0x0142, 'l' },
			// n
			{ 0x00F1, 'n' }, { 0x0144, 'n' }, { 0x0146, 'n' }, { 0x0148, 'n' },
			{ 0x0149, 'n' }, { 0x01F9, 'n' },
			// o
			{ 0x00F2, 'o' }, { 0x00F3, 'o' }, { 0x00F4, 'o' }, { 0x00F5, 'o' },
			{ 0x00F6, 'o' }, { 0x00F8, 'o' }, { 0x014D, 'o' }, { 0x014F, 'o' },
			{ 0x0151, 'o' }, { 0x01D2, 'o' }, { 0x01EB, 'o' }, { 0x01ED, 'o' },
			{ 0x020D, 'o' }, { 0x020F, 'o' }, { 0x022B, 'o' }, { 0x022D, 'o' },
			{ 0x022F, 'o' }, { 0x0231, 'o' },
			// r
			{ 0x0155, 'r' }, { 0x0157, 'r' }, { 0x0159, 'r' }, { 0x0211, 'r' },
			{ 0x0213, 'r' },
			// s
			{ 0x015B, 's' }, { 0x015D, 's' }, { 0x015F, 's' }, { 0x0161, 's' },
			{ 0x0219, 's' }, { 0x00DF, 's' }, // eszett -> 's' (lossy but conventional)
			// t
			{ 0x0163, 't' }, { 0x0165, 't' }, { 0x0167, 't' }, { 0x021B, 't' },
			// u
			{ 0x00F9, 'u' }, { 0x00FA, 'u' }, { 0x00FB, 'u' }, { 0x00FC, 'u' },
			{ 0x0169, 'u' }, { 0x016B, 'u' }, { 0x016D, 'u' }, { 0x016F, 'u' },
			{ 0x0171, 'u' }, { 0x0173, 'u' }, { 0x01D4, 'u' }, { 0x01D6, 'u' },
			{ 0x01D8, 'u' }, { 0x01DA, 'u' }, { 0x01DC, 'u' }, { 0x0215, 'u' },
			{ 0x0217, 'u' },
			// w
			{ 0x0175, 'w' },
			// y
			{ 0x00FD, 'y' }, { 0x00FF, 'y' }, { 0x0177, 'y' }, { 0x0233, 'y' },
			// z
			{ 0x017A, 'z' }, { 0x017C, 'z' }, { 0x017E, 'z' },
		};

		std::string output;
		output.reserve(input.size());

		for (size_t i = 0; i < input.size(); )
		{
			unsigned char c = input[i];
			uint32_t codepoint;
			int bytes;

			// Decode utf-8 codepoint
			if (c < 0x80)
			{
				codepoint = c;
				bytes = 1;
			}
			else if (c < 0xE0)
			{
				codepoint = c & 0x1F;
				bytes = 2;
			}
			else if (c < 0xF0)
			{
				codepoint = c & 0x0F;
				bytes = 3;
			}
			else
			{
				codepoint = c & 0x07;
				bytes = 4;
			}

			// Read remainder of codepoint
			for (int b = 1; b < bytes && (i + b) < input.size(); ++b)
				codepoint = (codepoint << 6) | (input[i + b] & 0x3F);

			if (const auto it = diacritic_map.find(codepoint); it != diacritic_map.end())
				output += it->second;
			else
				output.append(input, i, bytes);

			i += bytes;
		}

		return output;
	}

	fig::string int_to_string(int32_t value)
	{
		char buf[16];
		auto [parse_end, parse_err] = std::to_chars(buf, buf + sizeof(buf), value);
		return fig::string(buf, parse_end);
	}

	fig::string float_to_string(float value)
	{
		char buf[32];
		auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value,
			std::chars_format::general, 8);
		fig::string s(buf, ptr);
		if (s.find_first_of(".eEnNiI") == fig::string::npos)
			s += ".0";
		return s;
	}

	int32_t string_to_int(const fig::string_view& s, int32_t default_value)
	{
		int32_t value {};
		auto [ptr, err] = std::from_chars(s.data(), s.data() + s.size(), value);
		if (err != std::errc {} or ptr != s.data() + s.size())
			return default_value;
		return value;
	}

	float string_to_float(const fig::string_view& s, float default_value)
	{
		float value {};
		auto [ptr, err] = std::from_chars(s.data(), s.data() + s.size(), value);
		if (err != std::errc {} or ptr != s.data() + s.size())
			return default_value;
		return value;
	}

	std::optional<int32_t> string_to_int(const fig::string_view& s)
	{
		int32_t value {};
		auto [ptr, err] = std::from_chars(s.data(), s.data() + s.size(), value);
		if (err != std::errc {} or ptr != s.data() + s.size())
			return std::nullopt;
		return value;
	}

	std::optional<float> string_to_float(const fig::string_view& s)
	{
		float value {};
		auto [ptr, err] = std::from_chars(s.data(), s.data() + s.size(), value);
		if (err != std::errc {} or ptr != s.data() + s.size())
			return std::nullopt;
		return value;
	}

	fig::string unindent(const fig::string& s)
	{
		fig::string value { s };
		unindent_inplace(value);
		return value;
	}

	void unindent_inplace(fig::string& s)
	{
		size_t read = 0;
		size_t write = 0;
		size_t length = s.size();
		bool bLineStart = true;
		while (read < length)
		{
			char currentChar = s[read];
			if (bLineStart and (currentChar == ' ' or currentChar == '\t'))
			{
				++read;
				continue;
			}
			bLineStart = currentChar == '\n';
			s[write] = currentChar;
			++write;
			++read;
		}
		s.resize(write);
	}

	fig::string unescape(const fig::string& s) noexcept
	{
		fig::string value { s };
		unescape_inplace(value);
		return value;
	}

	void unescape_inplace(fig::string& s) noexcept
	{
		static const std::array<std::pair<char, char>, 5> escape_sequences {
			std::pair { 'n', '\n' },
			std::pair { 'r', '\r' },
			std::pair { 't', '\t' },
			std::pair { '\\', '\\' },
			std::pair { '_', ' ' },
		};

		size_t read = 0;
		size_t write = 0;
		size_t length = s.size();

		while (read < length)
		{
			char currentChar = s[read];
			if (currentChar == '\\' and read + 1 < length)
			{
				char nextChar = s[read + 1];
				
				bool replaced = false;
				for (auto& seq : escape_sequences)
				{
					if (nextChar == seq.first)
					{
						s[write] = seq.second;
						++write;
						read += 2;
						replaced = true;
						break;
					}
				}
				if (replaced)
					continue;
			}
			s[write] = currentChar;
			++write;
			++read;
		}
		s.resize(write);
	}

	fig::string escape(const fig::string& s) noexcept
	{
		fig::string value { s };
		escape_inplace(value);
		return value;
	}

	void escape_inplace(fig::string& s) noexcept
	{
		static const std::array<std::pair<char, char>, 4> escape_sequences {
			std::pair { '\n', 'n' },
			std::pair { '\r', 'r' },
			std::pair { '\t', 't' },
			std::pair { '\\', '\\' },
		};

		size_t length = s.size();

		// Count additional characters
		size_t added = 0;
		for (size_t i = 0; i < length; ++i)
		{
			for (auto& seq : escape_sequences)
			{
				if (s[i] == seq.first)
				{
					++added;
					break;
				}
			}
		}

		if (added == 0)
			return; // No escapes

		size_t newLength = s.size() + added;
		size_t read = length;
		size_t write = newLength;
		s.resize(newLength);

		while (read > 0)
		{
			--read;
			char ch = s[read];
			bool replaced = false;
			for (auto& seq : escape_sequences)
			{
				if (ch == seq.first)
				{
					s[--write] = seq.second;
					s[--write] = '\\';
					replaced = true;
					break;
				}
			}
			if (not replaced)
				s[--write] = ch;
		}
	}
}