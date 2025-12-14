#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;

namespace fig::llm
{
	struct LLMStatus;
}

class StatusBar : public Control
{
public:
	StatusBar(Control* pParent);

	void SetMessage(fig::string_view message);
	void SetModelInfo(fig::llm::LLMStatus status);

private:
	StaticText* _pMessage;
	StaticText* _pModelInfo;
};