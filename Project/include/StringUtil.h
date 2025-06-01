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

extern inline bool isEmpty(const std::string& s);

extern size_t validate_utf8(const std::string& text);
extern size_t string_find_partial_stop(const std::string_view& str, const std::string_view& stop);
extern size_t find_stopping_strings(const std::string& text, const std::vector<std::string>& stop_words, const size_t last_token_size, bool is_full_stop);
extern bool string_ends_with(const std::string_view& str, const std::string_view& suffix);

extern std::string get_filename(const std::string& str);

#endif