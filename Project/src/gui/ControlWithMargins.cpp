#include <pch.h>
#include "gui/ControlWithMargins.h"

using namespace fig::gui;

ControlWithMargins::ControlWithMargins(Control* pParent) : Control(pParent)
{
}

void ControlWithMargins::SetMargins(float left, float top, float right, float bottom)
{
	_marginLeft = left;
	_marginTop = top;
	_marginRight = right;
	_marginBottom = bottom;
}

void ControlWithMargins::SetMargins(Rectf rect) 
{
	SetMargins(rect.x, rect.y, rect.w, rect.h);
}

Rectf ControlWithMargins::GetClientRect() const noexcept
{
	Rectf clientRect = GetRect();
	clientRect.x += _marginLeft;
	clientRect.y += _marginTop;
	clientRect.w -= _marginLeft + _marginRight;
	clientRect.h -= _marginTop + _marginBottom;
	return clientRect;
}