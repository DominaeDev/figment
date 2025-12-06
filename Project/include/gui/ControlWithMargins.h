#ifndef CONTROL_WITH_MARGINS_H__
#define CONTROL_WITH_MARGINS_H__

#include "Control.h"

class ControlWithMargins : public Control
{
public:
	ControlWithMargins(Control* pParent);

	void SetMargins(int left, int top, int right, int bottom);
	void SetMargins(Rect rect);

protected:
	int _marginLeft = 0;
	int _marginTop = 0;
	int _marginRight = 0;
	int _marginBottom = 0;

	int HMargin() const { return _marginLeft + _marginRight; }
	int VMargin() const { return _marginTop + _marginBottom; }
};

#endif