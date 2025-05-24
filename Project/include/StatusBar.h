#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;

class StatusBar : public Control
{
public:
	StatusBar(Control* pParent);

	void SetMessage(string message);

private:
	StaticText* _pStatusText0;
};