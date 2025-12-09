export module AppState;

import Frame;
import <SDL3/SDL.h>;
import <SDL3_ttf/SDL_ttf.h>;

extern "C++" class LLMInstance;

export class Application
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

	static State* CreateState()
	{
		if (__appState)
			return __appState;

		__appState = (Application::State*)SDL_calloc(1, sizeof(Application::State));

		_pIBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);

		return __appState;
	}

	static void ReleaseState()
	{
		SDL_DestroyCursor(_pIBeamCursor);
	}

	static SDL_Window* GetWindow()
	{
		return __appState ? __appState->pWindow : nullptr;
	}

	static SDL_Renderer* GetRenderer()
	{
		return __appState ? __appState->pRenderer : nullptr;
	}

	static LLMInstance* GetLLM()
	{
		return __appState ? __appState->pLLM : nullptr;
	}

	static void SetCursor(SDL_SystemCursor cursor)
	{
		SDL_Cursor* pCursor;
		switch (cursor)
		{
		case SDL_SYSTEM_CURSOR_TEXT:
			pCursor = _pIBeamCursor;
			break;
		default:
			pCursor = SDL_GetDefaultCursor();
			break;
		}

		SDL_Cursor* pCurrentCursor = SDL_GetCursor();
		if (pCurrentCursor != pCursor)
			SDL_SetCursor(pCursor);
	}

private:
	static State* __appState;
	static SDL_Cursor* _pIBeamCursor;
};

export using AppState = Application::State;

Application::State* Application::__appState = nullptr;
SDL_Cursor* Application::_pIBeamCursor = nullptr;