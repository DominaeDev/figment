#include "gui/graphics.h"

SDL_FRect Rect_Expand(const SDL_FRect& rect, float pixels)
{
	return SDL_FRect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
}
