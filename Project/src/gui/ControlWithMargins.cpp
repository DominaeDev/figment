#include "gui/ControlWithMargins.h"

using namespace fig::gui;

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

void ControlWithMargins::SetMargins(Rect rect)
{
	SetMargins(rect.x, rect.y, rect.w, rect.h);
}