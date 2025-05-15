#include "AppState.h"
#include <SDL3/SDL.h>

Application::State* Application::__appState = nullptr;

Application::State* Application::CreateState()
{
	if (__appState)
		return __appState;

	__appState = (Application::State*)SDL_calloc(1, sizeof(Application::State));
	return __appState;
}

SDL_Window* Application::GetWindow()
{
	return __appState ? __appState->pWindow : nullptr;
}

SDL_Renderer* Application::GetRenderer()
{
	return __appState ? __appState->pRenderer : nullptr;
}