#pragma once

#include "Control.h"
#include "Types.h"

class StaticText;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, string name, string message, Role role, MessageType messageType);

	void SetMessage(const string& text);
	void AppendMessage(const string& text, bool lastPiece = false);

protected:
	void OnRender(SDL_Renderer* pRenderer) override;
	void OnSize() override;

private:
	string _name;
	string _message;
	StaticText* _pStaticText = nullptr;
	bool _bIgnoreEvent = false;
	Role _role = Role::Bot;
	MessageType _messageType = MessageType::Undefined;
};