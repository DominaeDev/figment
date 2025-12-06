#pragma once

#ifndef APP_STATE_H__
#define APP_STATE_H__

class Frame;
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Cursor;
enum SDL_SystemCursor : int;
struct TTF_TextEngine;
class LLMInstance;

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
		LLMInstance* pLLM;
	};

	static State* CreateState();
	static void ReleaseState();

	static SDL_Window* GetWindow();
	static SDL_Renderer* GetRenderer();
	static LLMInstance* GetLLM();

	static void SetCursor(SDL_SystemCursor cursor);

private:
	static State* __appState;

	static SDL_Cursor* _pIBeamCursor;

};

typedef Application::State AppState;

#endif