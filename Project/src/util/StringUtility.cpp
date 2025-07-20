#include "util/StringUtility.h"
#include <algorithm>

std::string& string_util::trim(std::string& s)
{
	return ltrim(rtrim(s));
}

std::string string_util::trim(const std::string& s)
{
	return ltrim(rtrim(s));
}

std::string& string_util::ltrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	return s;
}

std::string string_util::ltrim(const std::string& in)
{
	std::string s(in);
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	return s;
}

std::string& string_util::rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
	return s;
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

std::string& string_util::lcase(std::string& str)
{
	std::transform(std::begin(str), std::end(str), std::begin(str), [](unsigned char c){ return std::tolower(c); });
	return str;
}

std::string& string_util::ucase(std::string& str)
{
	std::transform(std::begin(str), std::end(str), std::begin(str), [](unsigned char c){ return std::tolower(c); });
	return str;
}

std::string string_util::lcase(const std::string& str)
{
	std::string s = str;
	std::transform(std::begin(str), std::end(str), std::begin(s), [](unsigned char c){ return std::tolower(c); });
	return s;
}

std::string string_util::ucase(const std::string& str)
{
	std::string s = str;
	std::transform(std::begin(str), std::end(str), std::begin(s), [](unsigned char c){ return std::tolower(c); });
	return s;
}

bool string_util::begins_with(const std::string_view& str, const std::string_view& suffix)
{
	return str.size() >= suffix.size() && str.compare(0, suffix.size(), suffix) == 0;
}

bool string_util::ends_with(const std::string_view& str, const std::string_view& suffix)
{
	return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
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

std::vector<std::string> string_util::split(std::string s, const std::string& delimiter)
{
	std::vector<std::string> tokens;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos)
	{
		token = s.substr(0, pos);
		tokens.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	tokens.push_back(s);

	return tokens;
}
