#pragma once

#include "gui/RenderTargetControl.h"
#include "chat/ChatTypes.h"
#include "chat/ChatSession.h"
#include "chat/MessagePoller.h"

#include <map>

namespace fig::gui
{
	class ChatMessage;
	class VerticalScrollSizer;
	class VerticalGradient;

	class ChatScroll : public RenderTargetControl
	{
	public:
		ChatScroll(control_ptr pParent);

		void SetSession(std::weak_ptr<fig::chat::ChatSession> pSession);

		void AddDummyMessage(string_cref name, fig::chat::Role role, fig::chat::MessageType msgType, string_cref message);
		void AddSystemMessage(string_cref message);

		int RemoveMessages(std::span<fig::uuid> ids);
		void ClearMessages();

		std::tuple<fig::uuid, fig::uuid> GetLastMessage() const;

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;
		void OnAfterLayout() override;
		void OnRenderMask(fig::renderer_ptr pRenderer, fig::sdl::Texture& texture) override;

	private:
		ChatMessage* AddMessage(const fig::uuid& characterId, fig::chat::Role role, fig::chat::MessageType msgType, string_cref message, bool complete);
		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void RefreshActive();

		void OnMessage(const fig::chat::MessagePoller::Message& msg);
	private:
		std::unique_ptr<VerticalGradient> _pBottomGradient;

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
		fig::observer_ptr<VerticalScrollSizer> _pScrollSizer;
		float _fScrollY = 0.0f;
		float _fLastListHeight = 0.0f;
		float _fAnimatedScroll = 0.0f;

		std::weak_ptr<fig::chat::ChatSession> _pSession {};
	};
}