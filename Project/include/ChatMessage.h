#pragma once

#include "Control.h"
#include "Types.h"
#include "Message.h"

class StaticText;

class ChatMessage : public Control
{
public:
	ChatMessage(Control* pParent, string name, string message, MessageType messageType);

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
	MessageType _messageType = MessageType::Undefined;
};