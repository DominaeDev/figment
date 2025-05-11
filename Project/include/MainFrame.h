#pragma once

#include "Frame.h"
class Sizer;

class MainFrame : public Frame
{
public:
	MainFrame(SDL_Window* pWindow);
	virtual ~MainFrame();

protected:
	virtual void OnUpdate(float fDeltaTime) override;
	virtual void OnRender(SDL_Renderer* pRenderer) override;
	
private:
	Sizer* _pSizer;
};