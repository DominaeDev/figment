#include "gui/graphics.h"

Rect expand_rect(const Rect& rect, int pixels)
{
	return Rect { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
}

Rectf expand_rect(const Rectf& rect, float pixels)
{
	return Rectf { rect.x - pixels, rect.y - pixels, rect.w + pixels * 2, rect.h + pixels * 2 };
}

Rect toRect(Rectf rect)
{
	return Rect { 
		(int32_t)rect.x, 
		(int32_t)rect.y, 
		(int32_t)rect.w, 
		(int32_t)rect.h
	};
}

Rectf toRectf(Rect rect)
{
	return Rectf { 
		(float)rect.x, 
		(float)rect.y, 
		(float)rect.w, 
		(float)rect.h 
	};
}