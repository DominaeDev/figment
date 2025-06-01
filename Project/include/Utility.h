#pragma once

#include "Types.h"

#define toI(X) static_cast<int>(X)
#define toF(X) static_cast<float>(X)

extern SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels);

template<typename T>
inline T clamp(const T& value, T min, T max)
{
	return std::min(std::max(value, std::min(min, max)), std::max(min, max));
}