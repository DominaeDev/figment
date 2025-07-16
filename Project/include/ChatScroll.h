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
	void ClearMessages();

protected:
	void OnUpdate(float fDeltaTime) override;
	void OnRender(SDL_Renderer* pRenderer) override {};

	void EnablePolling(bool bEnable);
	void Poll();

private:
	bool _bPolling = true;
	float _fPollTimer = 0.0f;

	struct MessageEntry
	{
		MessageType msgType;
		ChatMessage* pChatMessage;
	};
	std::map<uuid, MessageEntry> _messagesById {};
};