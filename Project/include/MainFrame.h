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

	void StartChat();

	static void SetStatusBar(string message);
	static MainFrame& GetInstance() { return *s_pInstance; }
	
	void EnableAutoChat(bool bEnable);
protected:
	virtual void OnUpdate(float fDeltaTime) override;
	virtual void OnRender(SDL_Renderer* pRenderer) override;

	void OnCommand(Command cmd);
	void RunAutomation();
	void PollStatus();

private:
	StatusBar* _pStatusBar;
	ChatScroll* _pChatScroll;
	bool _bAutoChat;
	std::queue<string> _autoQueue;
	float _fpollingCounter = 0.0f;
};
