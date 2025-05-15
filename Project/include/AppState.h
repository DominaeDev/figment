#pragma once

#ifndef __Application_h_
#define __Application_h_

struct SDL_Window;
struct SDL_Renderer;
class Frame;
struct TTF_TextEngine;

class Application
{
public:
	struct State
	{
		SDL_Window* pWindow;
		SDL_Renderer* pRenderer;
		Frame* pTopFrame;
		unsigned __int64 last_step;
		TTF_TextEngine* pTextEngine;
	};

	static State* CreateState();
	static SDL_Window* GetWindow();
	static SDL_Renderer* GetRenderer();

private:
	static State* __appState;
};

typedef Application::State AppState;

#endif