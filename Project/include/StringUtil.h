#pragma once

#ifndef STRING_UTIL_H__
#define STRING_UTIL_H__

#include <string>
#include <vector>

extern inline std::string& trim(std::string& s);
extern std::string& ltrim(std::string& s);
extern std::string& rtrim(std::string& s);
extern inline std::string trim(const std::string& s);
extern std::string ltrim(const std::string& s);
extern std::string rtrim(const std::string& s);

extern std::string lcase(const std::string& s);
extern std::string ucase(const std::string& s);

extern bool empty_or_whitespace(const std::string& s);

extern bool string_begins_with(const std::string_view& str, const std::string_view& prefix);
extern bool string_ends_with(const std::string_view& str, const std::string_view& suffix);

extern std::string get_filename(const std::string& str);

extern std::string& replace(std::string& str, const std::string& find, const std::string& replace);
extern std::string& replace_all(std::string& str, const std::string& find, const std::string& replace);

#endif