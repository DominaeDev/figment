#include "model/AppState.h"
#include "gui/MainFrame.h"
#include "llm/LLMEngine.h"
#include <SDL3/SDL.h>
#include <cassert>

using namespace fig::llm;

ApplicationState::State* ApplicationState::__appState = nullptr;
SDL_Cursor* ApplicationState::_pIBeamCursor = nullptr;

ApplicationState::State* ApplicationState::CreateState()
{
	if (__appState)
		return __appState;

	__appState = (ApplicationState::State*)SDL_calloc(1, sizeof(ApplicationState::State));

	_pIBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);

	__appState->pLLMEngine = std::make_shared<LLMEngine>();

	return __appState;
}

void ApplicationState::ReleaseState()
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

SDL_Window* ApplicationState::GetWindow()
{
	return __appState ? __appState->pWindow : nullptr;
}

SDL_Renderer* ApplicationState::GetRenderer()
{
	return __appState ? __appState->pRenderer : nullptr;
}

LLMEngine& ApplicationState::GetLLMEngine()
{
	assert(__appState);
	return *(__appState->pLLMEngine.get());
}

std::shared_ptr<LLMInstance> ApplicationState::GetLLMInstance()
{
	assert(__appState);
	return __appState->pLLMInstance;
}

void ApplicationState::SetLLMInstance(std::shared_ptr<LLMInstance> pLLMInstance)
{
	assert(__appState);
	__appState->pLLMInstance = pLLMInstance;
}

void ApplicationState::SetCursor(SDL_SystemCursor cursor)
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