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
	class Window;
	class Sizer;
	class StatusBar;
	class ChatScroll;
	class TextBox;
	class VariableList;
	class ChatSidePanel;

	class ChatScreen : public Screen
	{
		friend bool fig::chat::ChatCommandExecutor::Execute(fig::chat::ParsedChatCommand command, fig::chat::ChatCommandExecutor::Context context);
	public:
		ChatScreen(Frame* pParent);

		void StartChat(const fig::chat::ChatStaging& staging, fig::uuid instanceID);
		fig::observer_ptr<ChatSidePanel> GetSidePanel() { return _pSidePanel; }

	protected:
		virtual void OnUpdate(float fElapsed) override;
		virtual void OnRender(Renderer* pRenderer) override;

		EventResult OnEvent(Event& event) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;

		bool OnCommand(fig::chat::ParsedChatCommand cmd);
		void EnqueueCommand(fig::chat::ParsedChatCommand cmd);
		void NextQueuedCommand();

		void SetStatusBar(fig::string_view message);

	private:
		fig::observer_ptr<ChatScroll> _pChatScroll {};
		fig::observer_ptr<TextBox> _pTextBox {};
		fig::observer_ptr<VariableList> _pVariableList {};
		fig::observer_ptr<ChatSidePanel> _pSidePanel {};

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

}
