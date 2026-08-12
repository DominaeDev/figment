#include <pch.h>

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Figment.h"
#include "app/AppState.h"
#include "gui/Fonts.h"
#include "gui/MainFrame.h"
#include "gui/Window.h"
#include "gui/Events.h"
#include "llm/LLMBackend.h"
#include "llm/LLMInstance.h"
#include "audio/AudioManager.h"

#if defined(_DEBUG)
#define DETECT_MEMORY_LEAKS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#if _WIN32 && _DEBUG
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
static HANDLE shutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

BOOL WINAPI OnConsoleCtrl(DWORD controlType)
{
	// Handle console close event
	if (controlType == CTRL_CLOSE_EVENT
		or controlType == CTRL_C_EVENT
		or controlType == CTRL_BREAK_EVENT)
	{
		SDL_Event quitEvent = {};
		quitEvent.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quitEvent);

		WaitForSingleObject(shutdownEvent, 5000);
		return TRUE;
	}
	return FALSE;
}

#endif

// Set this to break on the specified allocation index. 0 = off
#define MEMORY_LEAK_ALLOC 0

#define APP_STATE(P) static_cast<AppState*>(P);

SDL_AppResult SDL_AppInit(void** ppAppState, int argc, char* argv[])
{
#ifdef DETECT_MEMORY_LEAKS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#if defined(MEMORY_LEAK_ALLOC) && MEMORY_LEAK_ALLOC > 0
	_CrtSetBreakAlloc(MEMORY_LEAK_ALLOC);
#endif
#endif

#if _WIN32 && _DEBUG
	SetConsoleCtrlHandler(OnConsoleCtrl, TRUE);
#endif

	SDL_SetHint(SDL_HINT_APP_NAME, "Figment");
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1"); // Generate mouse events for clicks that activate the window
	SDL_SetHint(SDL_HINT_MAC_OPTION_AS_ALT, "both");
	SDL_SetHint(SDL_HINT_IME_IMPLEMENTED_UI, "composition,candidates");
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
//	SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON, "0");
//	SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL, "0");

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!TTF_Init())
	{
		SDL_Log("Couldn't initialise SDL_ttf: %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	fig::gui::RegisterUserEvents();

	setlocale(LC_CTYPE, "");

	auto pAppState = fig::Global::CreateState();
	if (!pAppState)
		return SDL_APP_FAILURE;
	*ppAppState = pAppState;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* state, SDL_Event* event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	if (fig::Global::GetMainWindow().HandleEvent(*event))
	{
		return SDL_APP_CONTINUE;
	}

	return SDL_APP_CONTINUE;  /* carry on with the program! */
}

SDL_AppResult SDL_AppIterate(void* state)
{
	fig::AppState* pAppState = static_cast<fig::AppState*>(state);
    static Uint64 lastTick = 0;
    Uint64 now = SDL_GetTicks();
    Uint64 delta = now - lastTick;
	lastTick = now;

	float fElapsed = static_cast<float>(delta) / 1000.0f;

	fig::Global::GetLLMBackend().Update(fElapsed);
	fig::Global::GetAudioManager().Update(fElapsed);

	auto& mainWnd = fig::Global::GetMainWindow();
	mainWnd.Update(fElapsed);
	mainWnd.Render();

	if constexpr (Debugging)
	{
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
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* state, SDL_AppResult result)
{
	fig::gui::Fonts::ReleaseFonts();
	fig::Global::ReleaseState();

	TTF_Quit();
	SDL_Quit();

#if defined(_DEBUG) && defined(_MSC_VER)
	// https://github.com/microsoft/STL/issues/2504
	// to prevent the tzdb allocations from being reported as memory leaks
	std::chrono::get_tzdb_list().~tzdb_list();
#endif
}
