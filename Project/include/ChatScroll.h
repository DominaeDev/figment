#pragma once

#include "Control.h"
#include "Types.h"
#include "Message.h"

class ChatMessage;

class ChatScroll : public Control
{
public:
	ChatScroll(Control* pParent);

	ChatMessage* AddMessage(string name, string message, MessageType msgType);

	void StartListening();
	void StopListening();

protected:
	void OnUpdate(float fDeltaTime) override;
	void OnRender(SDL_Renderer* pRenderer) override {};

	void Poll();

	ChatMessage* _pLastBotMessage;
	bool _bListening = false;
	float _fListenTimer = 0.0f;

	int _messageId = 0;
	MessageType _messageType = MessageType::Undefined;

};