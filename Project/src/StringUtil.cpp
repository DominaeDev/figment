#include "StringUtil.h"
#include <algorithm>

std::string& trim(std::string& s)
{
	return ltrim(rtrim(s));
}

std::string trim(const std::string& s)
{
	return ltrim(rtrim(s));
}

std::string& ltrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	return s;
}

std::string ltrim(const std::string& in)
{
	std::string s(in);
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	return s;
}

std::string& rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
	return s;
}

std::string rtrim(const std::string& in)
{
	std::string s(in);
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
	return s;
}

inline bool isEmpty(const std::string& s)
{
	return s.size() == 0;
}

bool isEmptyOrWhitespace(const std::string& s)
{
	return s.size() == 0 || s.find_first_not_of(" \t\r\n", 0, 4) == std::string::npos;
}

std::string get_filename(const std::string& str)
{
	size_t pos = str.find_last_of("\\/");
	if (pos == std::string::npos)
		return str;
	return str.substr(pos + 1);
}

std::string& lcase(std::string& str)
{
	std::transform(std::begin(str), std::end(str), std::begin(str), [](unsigned char c){ return std::tolower(c); });
	return str;
}

std::string& ucase(std::string& str)
{
	std::transform(std::begin(str), std::end(str), std::begin(str), [](unsigned char c){ return std::tolower(c); });
	return str;
}

std::string lcase(const std::string& str)
{
	std::string s = str;
	std::transform(std::begin(str), std::end(str), std::begin(s), [](unsigned char c){ return std::tolower(c); });
	return s;
}

std::string ucase(const std::string& str)
{
	std::string s = str;
	std::transform(std::begin(str), std::end(str), std::begin(s), [](unsigned char c){ return std::tolower(c); });
	return s;
}

bool string_begins_with(const std::string_view& str, const std::string_view& suffix)
{
	return str.size() >= suffix.size() && str.compare(0, suffix.size(), suffix) == 0;
}

bool string_ends_with(const std::string_view& str, const std::string_view& suffix)
{
	return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string& replace_all(std::string& str, const std::string& find, const std::string& replace)
{
	auto&& pos = str.find(find);
	while (pos != std::string::npos)
	{
		str.replace(pos, find.length(), replace);
		pos = str.find(find, pos + replace.length());
	}
	return str;
}