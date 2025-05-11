#pragma once

#include "Types.h"

bool ColorIsDefined(SDL_Color color)
{
	return color.r != 0 || color.g != 0 || color.b != 0 || color.a != 0;
}