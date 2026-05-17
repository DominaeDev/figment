#pragma once

#include "gui/Control.h"
#include "chat/ChatTypes.h"
#include "chat/ChatSession.h"
#include <map>

namespace fig::gui
{
	class ChatMessage;
	class VerticalScrollSizer;

	class ChatScroll : public Control
	{
	public:
		ChatScroll(LayoutElement* pParent);

		void SetSession(fig::chat::ChatSession session) { _session = session; }

		void AddDummyMessage(StringCRef name, fig::chat::Role role, fig::chat::MessageType msgType, StringCRef message);
		void AddSystemMessage(StringCRef message);

		int RemoveMessages(std::span<fig::string> ids);
		void ClearMessages();

		std::tuple<fig::string, fig::string> GetLastMessage() const;

	protected:
		void OnUpdate(float fElapsed) override;
		bool OnEvent(Event& event) override;
		void OnAfterLayout() override;
		void OnAddedChild(LayoutElement* pChild) override;

	private:
		ChatMessage* AddMessage(const fig::uuid& characterId, fig::chat::Role role, fig::chat::MessageType msgType, StringCRef message, bool complete);
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
			fig::uuid characterId;
			fig::string chatId;
			fig::chat::Role role { fig::chat::Role::Undefined };
			fig::string responseId;
			fig::string subMessageId;
			fig::chat::MessageType msgType { fig::chat::MessageType::Undefined };
			ChatMessage* pChatMessage {};
		};
		std::vector<MessageEntry> _messages {};
		std::map<fig::string, MessageEntry*> _messagesById {}; // Sub-message id

		// Scrolling
		VerticalScrollSizer* _pScrollSizer;
		float _fScrollY = 0.0f;
		float _fLastListHeight = 0.0f;
		float _fAnimatedScroll = 0.0f;

		fig::chat::ChatSession _session {};
	};
}