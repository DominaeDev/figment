#pragma once

#include "Control.h"

class Frame : public Control
{
public:
	Frame(SDL_Window* pWindow);
	virtual ~Frame();

	SDL_Window* GetWindow() const { return _pWindow; }
	SDL_WindowID GetWindowID() const;

protected:
	bool OnEvent(SDL_Event* event) override;

	SDL_Window* _pWindow = nullptr;
};
