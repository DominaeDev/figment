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

#if PLATFORM_WINDOWS
#include "tts/TTSBackend_Win.h"
#endif
#include "audio/AudioManager.h"

using namespace fig::gui;
using namespace fig::io;
using namespace fig::llm;
using namespace fig::tts;
using namespace fig::audio;

namespace fig
{
	Global::State* Global::__appState = nullptr;

	static void BackendSignalHandler(const LLMStatus& signal);

	static void CreateCursor(fig::cursor, SDL_SystemCursor);

	void Global::State::Init()
	{
		// Load application settings
		pAppSettings = std::make_unique<AppSettings>(Constants::Paths::AppSettings);
		pAppSettings->Load();

		// Load cursors
		CreateCursor(SDL_SYSTEM_CURSOR_TEXT);
		CreateCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
		CreateCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);

		// Load user profiles
		pUserManager = std::make_shared<fig::user::UserManager>();
		if (not pUserManager->LoadProfiles())
			pUserManager->CreateDefaultProfile();

		// Load macros
		pMacroProvider = std::make_unique<fig::text::MacroProvider>(fig::path { Constants::Paths::Macros });

		// Create main frame
		auto windowSize = GetSettings().GetPoint2D(AppSetting::Interface::WindowSize);
		pMainWindow = std::make_shared<Window>(fig::strings::ApplicationTitle, 
			std::max(windowSize.x, Constants::GUI::WindowMinWidth), 
			std::max(windowSize.y, Constants::GUI::WindowMinHeight));
		pMainWindow->CreateFrame<MainFrame>();

#if !_DEBUG
		if (GetSettings().GetBool(AppSetting::Interface::WindowMaximized))
			SDL_MaximizeWindow(pMainWindow->GetSDLWindow().get());
#endif

		// Init LLM
		pLLMBackend = std::make_shared<LLMBackend>();
		pLLMBackend->RegisterObserver(BackendSignalHandler);

		// Init Audio
		pAudioManager = std::make_unique<AudioManager>();
		pAudioManager->SetVolume(0.8f);

		// Init TTS
		pTTSBackend = std::make_unique<TTSBackend>();
	}

	void Global::State::Release()
	{
		if (pLLMBackend)
			pLLMBackend->Shutdown();
		pLLMBackend.reset();
		pLLMInstance.reset();

		pAudioManager.reset();
		pTTSBackend.reset();

		pMainWindow.reset();
		pUserManager.reset();
		pMacroProvider.reset();

		if (pAppSettings)
			pAppSettings->Save();
		pAppSettings.reset();

		pSystemCursors.reset();
	}

	void Global::State::CreateCursor(SDL_SystemCursor cursor)
	{
		if (not (bool)pSystemCursors)
			pSystemCursors = std::make_unique<std::map<fig::cursor, fig::sdl::Cursor>>();
		(*pSystemCursors)[(fig::cursor)cursor] = std::move(fig::sdl::Cursor(cursor));
	}

	Global::State* Global::CreateState()
	{
		if (__appState)
			return __appState;

		__appState = (Global::State*)SDL_calloc(1, sizeof(Global::State));

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
	}

	Window& Global::GetMainWindow()
	{
		assert(__appState && __appState->pMainWindow);
		return *__appState->pMainWindow.get();
	}

	LLMBackend& Global::GetLLMBackend()
	{
		assert(__appState);
		return *(__appState->pLLMBackend.get());
	}

	ITTSBackend& Global::GetTTSBackend()
	{
		assert(__appState);
		return *(__appState->pTTSBackend.get());
	}

	AudioManager& Global::GetAudioManager()
	{
		assert(__appState);
		return *(__appState->pAudioManager.get());
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

	void Global::SetCursor(fig::cursor cursor)
	{
		assert(__appState);
		SDL_Cursor* pCursor = nullptr;
		if (auto itFind = __appState->pSystemCursors->find(cursor); itFind != std::end(*__appState->pSystemCursors))
			pCursor = itFind->second.get();
		if (!(bool)pCursor)
			pCursor = SDL_GetDefaultCursor();

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

		PushEvent(UserEvent::LLMStatusUpdate, 0, (void*)(&status));
		PushEvent(eventType);
	}
}