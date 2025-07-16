#pragma once

#include "Frame.h"
#include "Types.h"
#include "Command.h"
#include <queue>

class Sizer;
class StatusBar;
class ChatScroll;

class MainFrame : public Frame
{
	static MainFrame* s_pInstance;
public:
	MainFrame(SDL_Window* pWindow);
	virtual ~MainFrame();

	void LoadModel();
	void UnloadModel();

	static void SetStatusBar(string message);
	static MainFrame& GetInstance() { return *s_pInstance; }

	bool HandleKeyboardInput(SDL_Keycode key, bool down);

protected:
	virtual void OnUpdate(float fDeltaTime) override;
	virtual void OnRender(SDL_Renderer* pRenderer) override;

	void OnCommand(Command cmd);
	void PollStatus();
	void StartChat();

private:
	StatusBar* _pStatusBar;
	ChatScroll* _pChatScroll;
	float _fPollingCounter = 0.0f;
	bool _bStartedChat = false; // Used to trigger greeting

#if AUTOCHAT
public:
	void ToggleAutoChat();
private:
	void AutoChat();
	bool _bAutoChat;
	std::vector<string> _autoScript;
	size_t _autoScriptIndex = 0;
#endif
};
