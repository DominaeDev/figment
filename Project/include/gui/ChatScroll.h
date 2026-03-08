#pragma once

#include "gui/Control.h"
#include "model/ChatTypes.h"
#include "model/ChatSession.h"
#include <map>

namespace fig::gui
{
	class ChatMessage;
	class VerticalScrollSizer;

	class ChatScroll : public Control
	{
	public:
		ChatScroll(LayoutElement* pParent);

		void SetSession(fig::io::data::ChatSession session) { _session = session; }

		void AddDummyMessage(fig::string name, Role role, MessageType msgType, fig::string message);
		void AddSystemMessage(fig::string message);

		int RemoveMessages(std::span<fig::string> ids);
		void ClearMessages();

		std::tuple<fig::string, fig::string> GetLastMessage() const;

	protected:
		void OnUpdate(float fDeltaTime) override;
		bool OnEvent(Event& event) override;
		void OnAfterLayout() override;
		void OnAddedChild(LayoutElement* pChild) override;

	private:
		ChatMessage* AddMessage(fig::string name, Role role, MessageType msgType, fig::string message, bool complete);
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
			fig::string characterId;
			Role role;
			fig::string responseId;
			fig::string subMessageId;
			MessageType msgType;
			ChatMessage* pChatMessage;
		};
		std::vector<MessageEntry> _messages {};
		std::map<fig::string, MessageEntry*> _messagesById {}; // Sub-message id

		// Scrolling
		VerticalScrollSizer* _pScrollSizer;
		float _fScrollY = 0.0f;
		float _fLastListHeight = 0.0f;
		float _fAnimatedScroll = 0.0f;

		fig::io::data::ChatSession _session {};
	};
}