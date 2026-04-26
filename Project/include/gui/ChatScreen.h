#ifndef CHAT_FRAME_H__
#define CHAT_FRAME_H__
#pragma once

#include "Screen.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"

using ParsedChatCommandQueue = std::queue<ParsedChatCommand>;

namespace fig::io
{
	class ChatStaging;
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
		friend bool ChatCommandExecutor::Execute(ParsedChatCommand command, ChatCommandExecutor::Context context);
	public:
		ChatScreen(Frame* pParent);

		void Close();
		void StartChat();
		void StartChat(const fig::io::ChatStaging& staging);

	protected:
		virtual void OnUpdate(float fElapsed) override;
		virtual void OnRender(Renderer* pRenderer) override;

		bool OnEvent(Event& event) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;

		bool OnCommand(ParsedChatCommand cmd);
		void EnqueueCommand(ParsedChatCommand cmd);
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

		ParsedChatCommandQueue _commandQueue;

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