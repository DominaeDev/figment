#pragma once

#include "Frame.h"
#include "Types.h"

class Sizer;
class StatusBar;

class MainFrame : public Frame
{
	static MainFrame* s_pInstance;
public:
	MainFrame(SDL_Window* pWindow);
	virtual ~MainFrame();

	void LoadModel();

	static void SetStatusBar(string message);
	static MainFrame& GetInstance() { return *s_pInstance; }

protected:
	virtual void OnUpdate(float fDeltaTime) override;
	virtual void OnRender(SDL_Renderer* pRenderer) override;

private:
	StatusBar* _pStatusBar;
	
};
