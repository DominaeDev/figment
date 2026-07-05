#pragma once

#include "gui/Control.h"
#include "chat/ChatTypes.h"
#include "chat/ChatSession.h"
#include "chat/MessagePoller.h"

#include <map>

namespace fig::gui
{
	class ChatMessage;
	class VerticalScrollSizer;

	class ChatScroll : public Control
	{
	public:
		ChatScroll(LayoutElement* pParent);

		void SetSession(std::shared_ptr<fig::chat::ChatSession> pSession);

		void AddDummyMessage(string_cref name, fig::chat::Role role, fig::chat::MessageType msgType, string_cref message);
		void AddSystemMessage(string_cref message);

		int RemoveMessages(std::span<fig::uuid> ids);
		void ClearMessages();

		std::tuple<fig::uuid, fig::uuid> GetLastMessage() const;

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(Event& event) override;
		void OnAfterLayout() override;
		void OnAddedChild(LayoutElement* pChild) override;

	private:
		ChatMessage* AddMessage(const fig::uuid& characterId, fig::chat::Role role, fig::chat::MessageType msgType, string_cref message, bool complete);
		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void RefreshActive();

		void OnMessage(const fig::chat::MessagePoller::Message& msg);
	private:
		Control* _pBottomGradient;

		struct MessageEntry
		{
			fig::uuid characterId;
			fig::string chatId;
			fig::chat::Role role { fig::chat::Role::Undefined };
			fig::uuid responseId;
			fig::uuid subMessageId;
			fig::chat::MessageType msgType { fig::chat::MessageType::Undefined };
			ChatMessage* pChatMessage {};
		};
		std::vector<MessageEntry> _messages {};
		std::map<fig::uuid, MessageEntry*> _messagesById {}; // Sub-message id

		// Scrolling
		VerticalScrollSizer* _pScrollSizer;
		float _fScrollY = 0.0f;
		float _fLastListHeight = 0.0f;
		float _fAnimatedScroll = 0.0f;

		std::shared_ptr<fig::chat::ChatSession> _pSession {};
	};
}