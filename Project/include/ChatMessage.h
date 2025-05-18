#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, string name, string message);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(SDL_Renderer* pRenderer) override;

private:
	string _name;
	string _message;
	StaticText* _pStaticText = nullptr;
	bool _bIgnoreEvent = false;
};