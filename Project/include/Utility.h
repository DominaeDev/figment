#pragma once

#include "Types.h"

#define toI(X) static_cast<int>(X)
#define toF(X) static_cast<float>(X)

extern SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels);