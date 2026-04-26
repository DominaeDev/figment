#include <pch.h>
#include "model/AppState.h"
#include "model/GlobalStrings.h"
#include "model/UserManager.h"
#include "gui/MainFrame.h"
#include "gui/Window.h"
#include "gui/Events.h"
#include "gui/GUITypes.h"
#include "llm/LLMBackend.h"
#include "llm/LLMStatus.h"
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

	static void BackendSignalHandler(const LLMStatus& signal);

	Global::State* Global::CreateState()
	{
		if (__appState)
			return __appState;

		__appState = (Global::State*)SDL_calloc(1, sizeof(Global::State));

		_pIBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);

		__appState->pAppSettings = std::make_unique<AppSettings>(Constants::Paths::AppSettings);
		__appState->pAppSettings->Load();

		__appState->pLLMBackend = std::make_shared<LLMBackend>();
		__appState->pLLMBackend->RegisterObserver(BackendSignalHandler);

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
			if (GetSettings().GetBool(AppSetting::WindowMaximized))
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

		if (__appState->pUserManager)
			__appState->pUserManager.reset();

		if (__appState->pAppSettings)
		{
			__appState->pAppSettings->Save();
			__appState->pAppSettings.reset();
		}

		if (__appState->pLLMBackend)
		{
			__appState->pLLMBackend->Shutdown();
			__appState->pLLMBackend.reset();
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
		return *(__appState->pLLMBackend.get());
	}

	std::shared_ptr<LLMInstance> Global::GetLLMInstance()
	{
		assert(__appState);
		return __appState->pLLMInstance;
	}

	bool Global::IsLLMInitialized() 
	{ 
		assert(__appState);
		return __appState->pLLMBackend.get()->IsInitialized();
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

	fig::UserSettings& Global::GetUserSettings()
	{
		assert(__appState);
		return __appState->pUserManager.get()->GetSettings();
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

	static void BackendSignalHandler(const LLMStatus& status)
	{
		EventType eventType;
		switch (status.event)
		{
		case LLMStatusEvent::Nothing:						break;
		case LLMStatusEvent::ModelLoading:					eventType = EventType::LLMModelLoading; break;
		case LLMStatusEvent::ModelLoaded:					eventType = EventType::LLMModelLoaded; break;
		case LLMStatusEvent::ModelLoadFailure:				eventType = EventType::LLMModelLoadFailure; break;
		case LLMStatusEvent::ModelUnloaded:
		{
			eventType = EventType::LLMModelUnloaded;
			Global::SetLLMInstance(nullptr);
		}
		break;
		case LLMStatusEvent::ModelUnloadRequest:			eventType = EventType::LLMModelUnloadRequest; break;
		case LLMStatusEvent::ChatInitializing:				eventType = EventType::LLMChatInitializing; break;
		case LLMStatusEvent::ChatInitialized:				eventType = EventType::LLMChatInitialized; break;
		case LLMStatusEvent::ChatInitializationFailure:		eventType = EventType::LLMChatInitializationFailure; break;
		case LLMStatusEvent::GenerationStarted:				eventType = EventType::LLMGenerationStarted; break;
		case LLMStatusEvent::GenerationComplete:			eventType = EventType::LLMGenerationComplete; break;
		case LLMStatusEvent::CompletedMessage:				eventType = EventType::LLMCompletedMessage; break;
		case LLMStatusEvent::RebuildingKVCache:				eventType = EventType::LLMRebuildingKVCache; break;
		default:
			assert(false && "Missing signal handler");
			break;
		}

		fig::gui::PushEvent(EventType::LLMStatusUpdate, 0, (void*)(&status));
		fig::gui::PushEvent(eventType);
	}
}