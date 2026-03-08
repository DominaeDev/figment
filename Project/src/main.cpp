#include <pch.h>

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Types.h"
#include "Constants.h"
#include "model/AppState.h"
#include "gui/Fonts.h"
#include "gui/MainFrame.h"
#include "gui/Window.h"
#include "llm/LLMBackend.h"
#include "llm/LLMInstance.h"

#if defined(_DEBUG)
	#define DETECT_MEMORY_LEAKS
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

// Set this to break on the specified allocation index. 0 = off
#define MEMORY_LEAK_ALLOC 0

#define APP_STATE(P) static_cast<AppState*>(P);

using namespace fig::gui;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** ppAppState, int argc, char* argv[])
{
#ifdef DETECT_MEMORY_LEAKS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#if defined(MEMORY_LEAK_ALLOC) && MEMORY_LEAK_ALLOC > 0
	_CrtSetBreakAlloc(MEMORY_LEAK_ALLOC);
#endif
#endif

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!TTF_Init())
	{
		SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto pAppState = fig::Global::CreateState();
	if (!pAppState)
		return SDL_APP_FAILURE;
	*ppAppState = pAppState;

	return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* state, SDL_Event* event)
{
	fig::AppState* pAppState = static_cast<fig::AppState*>(state);

	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	if (pAppState->pMainWindow && pAppState->pMainWindow->HandleEvent(*event))
	{
		return SDL_APP_CONTINUE;
	}

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */

SDL_AppResult SDL_AppIterate(void* state)
{
	fig::AppState* pAppState = static_cast<fig::AppState*>(state);
    static Uint64 lastTick = 0;
    Uint64 now = SDL_GetTicks();
    Uint64 delta = now - lastTick;
	lastTick = now;

	float fElapsed = static_cast<float>(delta) / 1000.0f;

	if (pAppState->pMainWindow)
	{
		pAppState->pMainWindow->Update(fElapsed);
		pAppState->pMainWindow->Render();
	}

#if _DEBUG
	// Count fps
	static Uint64 accu = 0;
    static Uint64 last = 0;
    static Uint64 past = 0;
    Uint64 now_ns = SDL_GetTicksNS();
    Uint64 dt_ns = now_ns - past;

    if (now_ns - last > 999999999) 
	{
        last = now_ns;
		fig::Global::GetMainWindow().SetTitle(std::format("{} {} fps", fig::strings::ApplicationTitle, accu));
        accu = 0;
    }
    past = now_ns;
    accu += 1;
#endif

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void SDL_AppQuit(void* state, SDL_AppResult result)
{
	Fonts::ReleaseFonts();
	fig::Global::ReleaseState();

	TTF_Quit();
	SDL_Quit();
}

