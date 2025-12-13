#pragma once

#include "Frame.h"
#include "Types.h"
#include "model/ChatCommands.h"

#include <queue>

class Sizer;
class StatusBar;
class ChatScroll;
class TextBox;
class VariableList;

class MainFrame : public Frame
{
	static MainFrame* s_pInstance;
public:
	MainFrame(SDL_Window* pWindow);
	virtual ~MainFrame();

	void InitializeModel();
	void UnloadModel();

	static void SetStatusBar(string message);
	static MainFrame& GetInstance() { return *s_pInstance; }

	bool HandleKeyboardEvent(SDL_KeyboardEvent event);

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

	std::queue<ParsedChatCommand> _commandQueue;

#if AUTOCHAT
private:
	void AutoChat();
	bool _bAutoChat;
	std::vector<string> _autoScript;
	size_t _autoScriptIndex = 0;
#endif
};
