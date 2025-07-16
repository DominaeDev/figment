#pragma once

#include "Types.h"
#include <optional>

#define toI(X) static_cast<int>(X)
#define toF(X) static_cast<float>(X)

SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels);

void DebugPrint(string message);
void DebugPrintLn(string message = "");

UUIDv4::UUID CreateUUID();

string& NormalizeNewlines(string& text);
string NormalizeNewlines(string&& s);
std::optional<string> ReadTextFile(const string& filename, bool normalizeNewlines = true);
bool WriteTextFile(const string& filename, const string& content, bool append = false);

template<typename T>
inline T clamp(const T& value, T min, T max)
{
	return std::min(std::max(value, std::min(min, max)), std::max(min, max));
}


