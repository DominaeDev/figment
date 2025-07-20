#pragma once

#include <string>
#include <vector>

namespace string_util
{
	extern inline std::string& trim(std::string& s);
	extern std::string& ltrim(std::string& s);
	extern std::string& rtrim(std::string& s);
	extern inline std::string trim(const std::string& s);
	extern std::string ltrim(const std::string& s);
	extern std::string rtrim(const std::string& s);

	extern std::string& lcase(std::string& str);
	extern std::string lcase(const std::string& s);
	extern std::string& ucase(std::string& str);
	extern std::string ucase(const std::string& s);

	extern std::string& replace(std::string& str, const std::string& find, const std::string& replace);
	extern std::string& replace_all(std::string& str, const std::string& find, const std::string& replace);
	extern std::string get_filename(const std::string& str);

	extern bool empty_or_whitespace(const std::string& s);
	extern bool begins_with(const std::string_view& str, const std::string_view& prefix);
	extern bool ends_with(const std::string_view& str, const std::string_view& suffix);
	extern std::vector<std::string> split(std::string s, const std::string& delimiter);
}