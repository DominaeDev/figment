#include <pch.h>
#include "model/AppState.h"
#include "model/GlobalStrings.h"
#include "model/UserManager.h"
#include "gui/MainFrame.h"
#include "gui/Window.h"
#include "gui/GUITypes.h"
#include "llm/LLMBackend.h"
#include "Constants.h"
#include <SDL3/SDL.h>
#include <cassert>

using namespace fig::gui;
using namespace fig::io;
using namespace fig::llm;

namespace fig
{
	Global::State* Global::__appState = nullptr;
	SDL_Cursor* Global::_pIBeamCursor = nullptr;

	Global::State* Global::CreateState()
	{
		if (__appState)
			return __appState;

		__appState = (Global::State*)SDL_calloc(1, sizeof(Global::State));

		_pIBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);

		__appState->pAppSettings = std::make_unique<AppSettings>(Constants::Paths::AppSettings);
		__appState->pAppSettings->Load();

		__appState->pLLMEngine = std::make_shared<LLMBackend>();

		// Load user profiles
		__appState->pUserManager = std::make_shared<fig::user::UserManager>();
		if (not __appState->pUserManager->LoadProfiles())
			__appState->pUserManager->CreateDefaultProfile();

		try
		{
			// Create main frame
			__appState->pMainWindow = std::make_shared<Window>(fig::strings::ApplicationTitle, 
				GetSettings().GetIntVector<2>(AppSetting::WindowSize)[0], 
				GetSettings().GetIntVector<2>(AppSetting::WindowSize)[1]);
			__appState->pMainWindow->CreateFrame<MainFrame>();

#if !_DEBUG
			SDL_MaximizeWindow(__appState->pMainWindow->GetSDLWindow());
#endif
		}
		catch (...)
		{
			ReleaseState();
			return nullptr;
		}

		return __appState;
	}

	void Global::ReleaseState()
	{
		__appState->pMainWindow.reset();
		__appState->pLLMInstance.reset();

		if (__appState->pAppSettings)
		{
			__appState->pAppSettings->Save();
			__appState->pAppSettings.reset();
		}

		if (__appState->pUserManager)
			__appState->pUserManager.reset();

		if (__appState->pLLMEngine)
		{
			__appState->pLLMEngine->Shutdown();
			__appState->pLLMEngine.reset();
		}

		SDL_free(__appState);
		SDL_DestroyCursor(_pIBeamCursor);
		__appState = nullptr;
	}

	fig::gui::Window& Global::GetMainWindow()
	{
		assert(__appState && __appState->pMainWindow);
		return *__appState->pMainWindow.get();
	}

	LLMBackend& Global::GetLLMEngine()
	{
		assert(__appState);
		return *(__appState->pLLMEngine.get());
	}

	std::shared_ptr<LLMInstance> Global::GetLLMInstance()
	{
		assert(__appState);
		return __appState->pLLMInstance;
	}

	AppSettings& Global::GetSettings()
	{
		assert(__appState);
		return *(__appState->pAppSettings.get());
	}

	fig::user::UserManager& Global::GetUserManager()
	{
		assert(__appState);
		return *(__appState->pUserManager.get());
	}

	void Global::SetLLMInstance(std::shared_ptr<LLMInstance> pLLMInstance)
	{
		assert(__appState);
		__appState->pLLMInstance = pLLMInstance;
	}

	void Global::SetCursor(SDL_SystemCursor cursor)
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
}