#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;

class StatusBar : public Control
{
public:
	StatusBar(Control* pParent);

	void SetMessage(string message);
	void SetModelInfo(string modelName, size_t maxCtxSize, size_t usedCtxSize);

private:
	StaticText* _pMessage;
	StaticText* _pModelInfo;
};