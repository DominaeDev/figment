#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;
struct LLMStatus;

class StatusBar : public Control
{
public:
	StatusBar(Control* pParent);

	void SetMessage(std::string_view message);
	void SetModelInfo(LLMStatus status);

private:
	StaticText* _pMessage;
	StaticText* _pModelInfo;
};