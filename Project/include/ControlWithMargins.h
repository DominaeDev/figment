#pragma once

#include "Control.h"

class ControlWithMargins : public Control
{
public:
	ControlWithMargins(Control* pParent);

	void SetMargins(int left, int top, int right, int bottom);
	void SetMargins(SDL_Rect rect);

protected:
	int _marginLeft = 8;
	int _marginTop = 4;
	int _marginRight = 4;
	int _marginBottom = 6;

	int HMargin() const { return _marginLeft + _marginRight; }
	int VMargin() const { return _marginTop + _marginBottom; }
};