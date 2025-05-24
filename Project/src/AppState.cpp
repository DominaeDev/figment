#include "AppState.h"
#include <SDL3/SDL.h>

Application::State* Application::__appState = nullptr;
SDL_Cursor* Application::_pIBeamCursor = nullptr;

Application::State* Application::CreateState()
{
	if (__appState)
		return __appState;

	__appState = (Application::State*)SDL_calloc(1, sizeof(Application::State));

	_pIBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);

	return __appState;
}

void Application::ReleaseState()
{
	SDL_DestroyCursor(_pIBeamCursor);
}

SDL_Window* Application::GetWindow()
{
	return __appState ? __appState->pWindow : nullptr;
}

SDL_Renderer* Application::GetRenderer()
{
	return __appState ? __appState->pRenderer : nullptr;
}

LLMInstance* Application::GetLLM()
{
	return __appState ? __appState->pLLM : nullptr;
}

void Application::SetCursor(SDL_SystemCursor cursor)
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