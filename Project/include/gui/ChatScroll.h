#pragma once

#include "gui/Control.h"
#include "model/ChatSession.h"
#include "llm/LLMTypes.h"

class ChatMessage;
class VerticalScrollSizer;

class ChatScroll : public Control
{
public:
	ChatScroll(Control* pParent);
	
	void SetSession(std::shared_ptr<ChatSession> session) { _pSession = session; }
	
	void AddDummyMessage(string name, Role role, MessageType msgType, string message);
	int RemoveMessages(std::vector<string> ids);
	void ClearMessages();

	std::tuple<std::string, std::string> GetLastMessage() const;

protected:
	void OnUpdate(float fDeltaTime) override;
	bool OnEvent(SDL_Event* event) override;
	void OnAfterLayout() override;
	void OnAddedChild(LayoutElement* pChild) override;

private:
	ChatMessage* AddMessage(string name, Role role, MessageType msgType, string message, bool complete);
	bool HandleMouseWheel(SDL_MouseWheelEvent event);
	void EnablePolling(bool bEnable);
	void Poll();
	void RefreshActive();

private:
	Control* _pBottomGradient;
	bool _bPolling = true;
	float _fPollTimer = 0.0f;
	
	struct MessageEntry
	{
		Role role;
		string responseId;
		string subMessageId;
		MessageType msgType;
		ChatMessage* pChatMessage;
	};
	std::vector<MessageEntry> _messages {};
	std::map<string, MessageEntry*> _messagesById {}; // Sub-message id

	// Scrolling
	VerticalScrollSizer* _pScrollSizer;
	float _fScrollY = 0.0f;
	float _fLastListHeight = 0.0f;
	float _fAnimatedScroll = 0.0f;

	std::shared_ptr<ChatSession> _pSession {};
};