#ifndef CHAT_FRAME_H__
#define CHAT_FRAME_H__
#pragma once

#include "Screen.h"
#include "chat/ChatCommands.h"
#include "chat/ChatStaging.h"
#include "chat/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"

namespace fig::chat
{
	class ChatStaging;
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

	class ChatScreen : public Screen
	{
		friend bool fig::chat::ChatCommandExecutor::Execute(fig::chat::ParsedChatCommand command, fig::chat::ChatCommandExecutor::Context context);
	public:
		ChatScreen(Frame* pParent);

		void Close();
		void StartChat(const fig::chat::ChatStaging& staging);

	protected:
		virtual void OnUpdate(float fElapsed) override;
		virtual void OnRender(Renderer* pRenderer) override;

		bool OnEvent(Event& event) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;

		bool OnCommand(fig::chat::ParsedChatCommand cmd);
		void EnqueueCommand(fig::chat::ParsedChatCommand cmd);
		void NextQueuedCommand();

		void SetStatusBar(fig::string_view message);
		void OnSidePanel(bool show) override;

	private:

		ChatScroll* _pChatScroll {};
		TextBox* _pTextBox {};
		VariableList* _pVariableList {};
		Control* _pExpandButton {};

		float _fPollingCounter = 0.0f;
		bool _bStartedChat = false; // Used to trigger greeting

		fig::chat::ParsedChatCommandQueue _commandQueue;

#if ENABLE_AUTO_CHAT
	private:
		void AutoChat();
		bool _bAutoChat {};
		std::vector<fig::string> _autoScript;
		size_t _autoScriptIndex = 0;
#endif
	};

}

#endif