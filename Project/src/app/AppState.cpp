#include <pch.h>
#include "app/AppState.h"
#include "user/UserManager.h"
#include "text/MacroProvider.h"
#include "gui/MainFrame.h"
#include "gui/Window.h"
#include "gui/Events.h"
#include "gui/GUITypes.h"
#include "llm/LLMBackend.h"
#include "llm/LLMStatus.h"
#include "app/Constants.h"
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

	void Global::State::Init()
	{
		// Load application settings
		pAppSettings = std::make_unique<AppSettings>(Constants::Paths::AppSettings);
		pAppSettings->Load();

		// Load user profiles
		pUserManager = std::make_shared<fig::user::UserManager>();
		if (not pUserManager->LoadProfiles())
			pUserManager->CreateDefaultProfile();

		// Load macros
		pMacroProvider = std::make_unique<fig::text::MacroProvider>(fig::path { Constants::Paths::Macros });

		// Create main frame
		pMainWindow = std::make_shared<Window>(fig::strings::ApplicationTitle,
			GetSettings().GetIntVector<2>(AppSetting::WindowSize)[0],
			GetSettings().GetIntVector<2>(AppSetting::WindowSize)[1]);
		pMainWindow->CreateFrame<MainFrame>();

#if !_DEBUG
		if (GetSettings().GetBool(AppSetting::WindowMaximized))
			SDL_MaximizeWindow(pMainWindow->GetSDLWindow().get());
#endif

		// Init LLM
		pLLMBackend = std::make_shared<LLMBackend>();
		pLLMBackend->RegisterObserver(BackendSignalHandler);
	}

	void Global::State::Release()
	{
		if (pLLMBackend)
			pLLMBackend->Shutdown();
		pLLMBackend.reset();

		pMainWindow.reset();
		pLLMInstance.reset();

		pUserManager.reset();
		pMacroProvider.reset();

		if (pAppSettings)
			pAppSettings->Save();
		pAppSettings.reset();
	}

	Global::State* Global::CreateState()
	{
		if (__appState)
			return __appState;

		__appState = (Global::State*)SDL_calloc(1, sizeof(Global::State));

		_pIBeamCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);

		try
		{
			__appState->Init();
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
		if (__appState)
		{
			__appState->Release();
			SDL_free(__appState);
			__appState = nullptr;
		}

		SDL_DestroyCursor(_pIBeamCursor);
		_pIBeamCursor = nullptr;
	}

	fig::gui::Window& Global::GetMainWindow()
	{
		assert(__appState && __appState->pMainWindow);
		return *__appState->pMainWindow.get();
	}

	LLMBackend& Global::GetLLMBackend()
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

	bool Global::IsSignedIn() 
	{ 
		assert(__appState);
		return __appState->pUserManager.get()->IsSignedIn();
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

	fig::io::UserContentManager& Global::GetUserContent()
	{
		assert(__appState);
		return GetUserManager().GetContent();
	}

	UserSettings& Global::GetUserSettings()
	{
		assert(__appState);
		return __appState->pUserManager.get()->GetSettings();
	}

	std::weak_ptr<fig::text::MacroProvider> Global::GetMacroProvider()
	{
		assert(__appState);
		return __appState->pMacroProvider;
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
		UserEvent eventType;
		switch (status.event)
		{
		case LLMStatusEvent::Nothing:						return;
		case LLMStatusEvent::ModelLoading:					eventType = UserEvent::LLMModelLoading; break;
		case LLMStatusEvent::ModelLoaded:					eventType = UserEvent::LLMModelLoaded; break;
		case LLMStatusEvent::ModelLoadFailure:				eventType = UserEvent::LLMModelLoadFailure; break;
		case LLMStatusEvent::ModelUnloaded:
			eventType = UserEvent::LLMModelUnloaded;
			Global::SetLLMInstance(nullptr);
			break;
		case LLMStatusEvent::ModelUnloadRequest:			eventType = UserEvent::LLMModelUnloadRequest; break;
		case LLMStatusEvent::ChatInitializing:				eventType = UserEvent::LLMChatInitializing; break;
		case LLMStatusEvent::ChatInitialized:				eventType = UserEvent::LLMChatInitialized; break;
		case LLMStatusEvent::ChatInitializationFailure:		eventType = UserEvent::LLMChatInitializationFailure; break;
		case LLMStatusEvent::ChatUnloaded:					eventType = UserEvent::LLMChatUnloaded; break;
		case LLMStatusEvent::GenerationStarted:				eventType = UserEvent::LLMGenerationStarted; break;
		case LLMStatusEvent::GenerationComplete:			eventType = UserEvent::LLMGenerationComplete; break;
		case LLMStatusEvent::CompletedMessage:				eventType = UserEvent::LLMCompletedMessage; break;
		case LLMStatusEvent::RebuildingKVCache:				eventType = UserEvent::LLMRebuildingKVCache; break;
		default:
			assert(false && "Missing signal handler");
			return;
		}

		fig::gui::PushEvent(UserEvent::LLMStatusUpdate, 0, (void*)(&status));
		fig::gui::PushEvent(eventType);
	}
}