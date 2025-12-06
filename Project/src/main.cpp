#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Constants.h"
#include "model/AppState.h"
#include "gui/Fonts.h"
#include "gui/TextureStore.h"
#include "gui/CharacterImageStore.h"
#include "gui/Text.h"
#include "gui/MainFrame.h"
#include "llm/LLMInstance.h"

#if defined(_DEBUG)
#define CHECK_MEMORY_LEAKS
#endif

#ifdef CHECK_MEMORY_LEAKS
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define MEMORY_LEAK_ALLOC 0

#define APP_STATE(P) static_cast<AppState*>(P);

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** ppAppState, int argc, char* argv[])
{
#ifdef CHECK_MEMORY_LEAKS
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

	SDL_Window* pWindow;
	Renderer* pRenderer;
	try
	{
		if (!SDL_CreateWindowAndRenderer(Constants::AppTitle, Constants::GUI::WindowWidth, Constants::GUI::WindowHeight, SDL_WINDOW_RESIZABLE, &pWindow, &pRenderer))
		{
			SDL_Log("Couldn't create pWindow/pRenderer: %s", SDL_GetError());
			return SDL_APP_FAILURE;
		}
	}
	catch (...)
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
#if !_DEBUG
	SDL_MaximizeWindow(pWindow);
#endif
	// Create TTF text engine
	pAppState->pTextEngine = Text::InitEngine(pRenderer);

	// Load fonts
	Fonts::Init();

	// Load textures
	TextureStore::Init(pRenderer);

	// Load character images (temp)
	CharacterImageStore::Init(pRenderer);

	// Create main frame
	auto pMainFrame = new MainFrame(pWindow);
	pAppState->pTopFrame = pMainFrame;

	// Instantiate LLM
	auto pLLM = new LLMInstance();
	pAppState->pLLM = pLLM;

	return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* state, SDL_Event* event)
{
	AppState* pAppState = static_cast<AppState*>(state);

	if (event->type == SDL_EVENT_QUIT)
	{
		Application::GetLLM()->Halt();
		return SDL_APP_SUCCESS;
	}

	if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
	{	
		if (static_cast<MainFrame*>(pAppState->pTopFrame)->HandleKeyboardEvent(event->key))
			return SDL_APP_CONTINUE;
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

	auto pRenderer = pAppState->pRenderer;
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

		SDL_SetWindowTitle(pAppState->pWindow, title);
    }
    past = now_ns;
    accu += 1;
#endif

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void SDL_AppQuit(void* state, SDL_AppResult result)
{
	AppState* pAppState = static_cast<AppState*>(state);
	delete pAppState->pTopFrame;
	delete pAppState->pLLM;
	
	CharacterImageStore::Release();
	TextureStore::Release();
	Fonts::ReleaseFonts();
	TTF_DestroyRendererTextEngine(pAppState->pTextEngine);
	TTF_Quit();
}

