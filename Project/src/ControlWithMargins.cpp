#include "ControlWithMargins.h"

ControlWithMargins::ControlWithMargins(Control* pParent) : Control(pParent)
{
}

void ControlWithMargins::SetMargins(int left, int top, int right, int bottom)
{
	_marginLeft = left;
	_marginTop = top;
	_marginRight = right;
	_marginBottom = bottom;
}

void ControlWithMargins::SetMargins(SDL_Rect rect)
{
	SetMargins(rect.x, rect.y, rect.w, rect.h);
}