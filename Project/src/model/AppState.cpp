#include "model/AppState.h"
#include "gui/MainFrame.h"
#include "llm/LLMEngine.h"
#include <SDL3/SDL.h>
#include <cassert>

Application::State* Application::__appState = nullptr;
SDL_Cursor* Application::_pIBeamCursor = nullptr;

Application::State* Application::CreateState()
{
	if (__appState)
		return __appState;

	__appState = (Application::State*)SDL_calloc(1, sizeof(Application::State));

	_pIBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);

	__appState->pLLMEngine = std::make_shared<LLMEngine>();

	return __appState;
}

void Application::ReleaseState()
{
	delete __appState->pTopFrame;
	
	__appState->pLLMInstance.reset();
	if (__appState->pLLMEngine)
	{
		__appState->pLLMEngine->Shutdown();
		__appState->pLLMEngine.reset();
	}

	SDL_free(__appState);
	SDL_DestroyCursor(_pIBeamCursor);
	__appState = nullptr;
}

SDL_Window* Application::GetWindow()
{
	return __appState ? __appState->pWindow : nullptr;
}

SDL_Renderer* Application::GetRenderer()
{
	return __appState ? __appState->pRenderer : nullptr;
}

LLMEngine& Application::GetLLMEngine()
{
	assert(__appState);
	return *(__appState->pLLMEngine.get());
}

std::shared_ptr<LLMInstance> Application::GetLLMInstance()
{
	assert(__appState);
	return __appState->pLLMInstance;
}

void Application::SetLLMInstance(std::shared_ptr<LLMInstance> pLLMInstance)
{
	assert(__appState);
	__appState->pLLMInstance = pLLMInstance;
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