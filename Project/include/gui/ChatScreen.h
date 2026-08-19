#pragma once

#include "Screen.h"
#include "chat/ChatCommands.h"
#include "chat/ChatStaging.h"
#include "chat/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"

namespace fig::chat
{
	class ChatStaging;
	class ChatSession;
	using ParsedChatCommandQueue = std::queue<ParsedChatCommand>;
}

namespace fig::gui
{
	class VariableList;
	class ChatScroll;
	class InfoPanel;
	class ChatBackground;
	class BehindChat;

	class ChatScreen : public Screen
	{
		friend bool fig::chat::ChatCommandExecutor::Execute(fig::chat::ParsedChatCommand command, fig::chat::ChatCommandExecutor::Context context);
	public:
		ChatScreen(Frame* pParent);

		void StartChat(const fig::chat::ChatStaging& staging, fig::uuid chatInstanceId, fig::uuid chatLogId);
		fig::observer_ptr<InfoPanel> GetSidePanel() { return _pInfoPanel; }
		fig::observer_ptr<ChatBackground> GetBackground() { return _pBackground; }

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		void OnAfterLayout() override;

		EventResult OnEvent(fig::event& event) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;

		bool OnCommand(fig::chat::ParsedChatCommand cmd);
		void EnqueueCommand(fig::chat::ParsedChatCommand cmd);
		void NextQueuedCommand();

		void SetStatusBar(fig::string_view message);

	private:
		fig::observer_ptr<ChatScroll> _pChatScroll {};
		fig::observer_ptr<BehindChat> _pBehindChat {};
		fig::observer_ptr<TextInput> _pTextBox {};
		fig::observer_ptr<VariableList> _pVariableList {};
		fig::observer_ptr<InfoPanel> _pInfoPanel {};
		fig::observer_ptr<ChatBackground> _pBackground {};

		float _fPollingCounter = 0.0f;
		bool _bStartedChat = false; // Used to trigger greeting

		fig::chat::ParsedChatCommandQueue _commandQueue;
		std::shared_ptr<fig::chat::ChatSession> _pSession;

#if ENABLE_AUTO_CHAT
	private:
		void AutoChat();
		bool _bAutoChat {};
		std::vector<fig::string> _autoScript;
		size_t _autoScriptIndex = 0;
#endif
	};

	template <>
	constexpr ScreenType ScreenTypeOf<ChatScreen> = ScreenType::Chat;
}
