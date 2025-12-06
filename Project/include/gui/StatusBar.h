#ifndef STATUS_BAR_H__
#define STATUS_BAR_H__

#include "Control.h"
#include "Types.h"

class StaticText;
struct LLMStatus;

class StatusBar : public Control
{
public:
	StatusBar(Control* pParent);

	void SetMessage(string message);
	void SetModelInfo(LLMStatus status);

private:
	StaticText* _pMessage;
	StaticText* _pModelInfo;
};

#endif