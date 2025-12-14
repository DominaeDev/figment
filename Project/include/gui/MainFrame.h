#pragma once

#include "Frame.h"
#include "Types.h"
#include "model/ChatCommands.h"
#include "model/ChatCommandExecutor.h"
#include "llm/LLMStatus.h"

class Sizer;
class StatusBar;
class ChatScroll;
class TextBox;
class VariableList;

using ParsedChatCommandQueue = std::queue<ParsedChatCommand>;

class MainFrame : public Frame
{
	friend bool ChatCommandExecutor::Execute(ParsedChatCommand command, ChatCommandExecutor::Context context);
	static MainFrame* s_pInstance;
public:
	MainFrame(SDL_Window* pWindow);
	virtual ~MainFrame();

	void InitializeModel();
	void UnloadModel();

	static void SetStatusBar(fig::string_view message);
	static MainFrame& GetInstance() { return *s_pInstance; }

	bool HandleKeyboardEvent(SDL_KeyboardEvent event);
	void Close();

protected:
	virtual void OnUpdate(float fDeltaTime) override;
	virtual void OnRender(Renderer* pRenderer) override;

	void PollStatus();
	void StartChat();
	bool OnCommand(ParsedChatCommand cmd);
	void EnqueueCommand(ParsedChatCommand cmd);
	void NextQueuedCommand();

private:
	StatusBar* _pStatusBar;
	ChatScroll* _pChatScroll;
	TextBox* _pTextBox;
	VariableList* _pVariableList;

	float _fPollingCounter = 0.0f;
	bool _bStartedChat = false; // Used to trigger greeting

	ParsedChatCommandQueue _commandQueue;

#if ENABLE_AUTO_CHAT
private:
	void AutoChat();
	bool _bAutoChat;
	std::vector<fig::string> _autoScript;
	size_t _autoScriptIndex = 0;
#endif
};
