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
	int RemoveMessages(std::vector<string> ids);
	void ClearMessages();

	std::tuple<std::string, std::string> GetLastMessage() const;

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
		string responseId;
		string subMessageId;
		MessageType msgType;
		ChatMessage* pChatMessage;
	};
	std::vector<MessageEntry> _messages {};
	std::map<string, MessageEntry*> _messagesById {}; // Sub-message id
};