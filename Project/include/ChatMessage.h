#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, string name, string message);

	void SetMessage(const string& text);
	void AppendMessage(const string& text);

protected:
	void OnRender(SDL_Renderer* pRenderer) override;
	void OnSize() override;

private:
	string _name;
	string _message;
	StaticText* _pStaticText = nullptr;
	bool _bIgnoreEvent = false;
};