#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "AppState.h"
#include "Types.h"
#include "Constants.h"
#include "Fonts.h"
#include "Text.h"
#include "MainFrame.h"
#include "Inference.h"

#define CHECK_MEMORY_LEAKS (defined(_DEBUG) && 1)

#define DEFAULT_MODEL_LOCATION "F:\\AI\\Models\\ana-v1-m7.erp.unc.Q6_K.gguf"

#if CHECK_MEMORY_LEAKS
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* pWindow = NULL;
static SDL_Renderer* pRenderer = NULL;

#define APP_STATE(P) static_cast<AppState*>(P);


/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** ppAppState, int argc, char* argv[])
{
#if CHECK_MEMORY_LEAKS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetBreakAlloc(281);
#endif

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

	if (!TTF_Init())
	{
		SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	AppState* pAppState = Application::CreateState();
	if (!pAppState)
		return SDL_APP_FAILURE;

	*ppAppState = pAppState;
	pAppState->pWindow = pWindow;
	pAppState->pRenderer = pRenderer;

	SDL_SetWindowMinimumSize(pWindow, 800, 400);
	SDL_SetRenderVSync(pRenderer, 1);

	// Create TTF text engine
	pAppState->pTextEngine = Text::InitEngine(pRenderer);

	// Load fonts
	Fonts::Init();

	// Create main frame
	auto pMainFrame = new MainFrame(pWindow);
	pAppState->pTopFrame = pMainFrame;

	Inference::Initialize();

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
	if (event->type == SDL_EVENT_KEY_DOWN)
	{	
		switch (event->key.key)
		{
		case SDLK_F2:
			if (!Inference::HasLoadedModel())
				Inference::LoadModel(DEFAULT_MODEL_LOCATION);
			break;
		}
	}

	if (pAppState->pTopFrame != nullptr)
		pAppState->pTopFrame->ProcessEvent(event);

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


#if _DEBUG
	// Count fps
	static Uint64 accu = 0;
    static Uint64 last = 0;
    static Uint64 past = 0;
    Uint64 now_ns = SDL_GetTicksNS();
    Uint64 dt_ns = now_ns - past;

	static char title[256];
    if (now_ns - last > 999999999) {
        last = now_ns;
        SDL_snprintf(title, sizeof(title), "%s %" SDL_PRIu64 " fps", Constants::AppTitle, accu);
        accu = 0;

		SDL_SetWindowTitle(pWindow, title);
    }
    past = now_ns;
    accu += 1;
#endif

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void SDL_AppQuit(void* state, SDL_AppResult result)
{
	Inference::Shutdown();

	AppState* pAppState = static_cast<AppState*>(state);
	delete pAppState->pTopFrame;
	
	Fonts::ReleaseFonts();
	TTF_DestroyRendererTextEngine(pAppState->pTextEngine);
	TTF_Quit();
}

