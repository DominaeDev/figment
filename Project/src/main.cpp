#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Types.h"
#include "Constants.h"
#include "MainFrame.h"

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* pWindow = NULL;
static SDL_Renderer* pRenderer = NULL;
static MainFrame* pMainFrame = nullptr;

#define APP_STATE(P) static_cast<AppState*>(P);

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** ppAppState, int argc, char* argv[])
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_CreateWindowAndRenderer(Constants::AppTitle, Constants::WindowWidth, Constants::WindowHeight, SDL_WINDOW_RESIZABLE, &pWindow, &pRenderer))
	{
		SDL_Log("Couldn't create pWindow/pRenderer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	AppState* pAppState = (AppState*)SDL_calloc(1, sizeof(AppState));
	if (!pAppState)
		return SDL_APP_FAILURE;

	*ppAppState = pAppState;

	int w, h;
	SDL_GetWindowSizeInPixels(pWindow, &w, &h);
	SDL_SetWindowMinimumSize(pWindow, 800, 400);

	// Create main frame
	pMainFrame = new MainFrame();
	pAppState->pTopFrame = pMainFrame;
	pMainFrame->SetSize((float)w, (float)h);

	return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* state, SDL_Event* event)
{
	auto pAppState = APP_STATE(state);

	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
	}
	else if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
	{
		if (pAppState->pTopFrame != nullptr)
			pAppState->pTopFrame->SetSize((float)event->window.data1, (float)event->window.data2);
	}
	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* state)
{
	AppState* pAppState = static_cast<AppState*>(state);
    static Uint64 lastTick = 0;
    Uint64 now = SDL_GetTicks();
    Uint64 delta = now - lastTick;
	lastTick = now;

	float fDeltaTime = static_cast<float>(delta) / 1000.0f;

	SDL_SetRenderDrawColor(pRenderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(pRenderer);

	if (pAppState->pTopFrame != nullptr)
	{
		pAppState->pTopFrame->Update(fDeltaTime);
		pAppState->pTopFrame->Render(pRenderer);
	}

	SDL_RenderPresent(pRenderer);

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	delete pMainFrame;
}

