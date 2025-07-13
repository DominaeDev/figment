#pragma once

#include "Types.h"
#include <optional>

#define toI(X) static_cast<int>(X)
#define toF(X) static_cast<float>(X)

extern SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels);

extern string& NormalizeNewlines(string& text);
extern string NormalizeNewlines(string&& s);
extern std::optional<string> ReadTextFile(const string& filename, bool normalizeNewlines = true);
extern bool WriteTextFile(const string& filename, const string& content, bool append = false);

template<typename T>
inline T clamp(const T& value, T min, T max)
{
	return std::min(std::max(value, std::min(min, max)), std::max(min, max));
}

