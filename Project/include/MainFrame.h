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
	
	void EnableAutomation(bool bEnable);
protected:
	virtual void OnUpdate(float fDeltaTime) override;
	virtual void OnRender(SDL_Renderer* pRenderer) override;

	void OnCommand(Command cmd);
	void RunAutomation();

private:
	StatusBar* _pStatusBar;
	ChatScroll* _pChatScroll;
	bool _bAutomation;
	std::queue<string> _autoQueue;
};
