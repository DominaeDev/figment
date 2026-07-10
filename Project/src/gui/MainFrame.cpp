#include <pch.h>
#include "gui/MainFrame.h"
#include "gui/HomeScreen.h"
#include "gui/ChatScreen.h"
#include "gui/DebugScreen.h"
#include "gui/ChatListingScreen.h"
#include "gui/SidePanel.h"
#include "gui/LoginScreen.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/FileUtility.h"
#include "llm/LLMBackend.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "data/ModelSettings.h"
#include "data/ChatInstance.h"
#include "util/DebugUtils.h"

using namespace fig::io;
using namespace fig::data;
using namespace fig::chat;

namespace fig::gui
{
	MainFrame* MainFrame::s_pInstance = nullptr;

	MainFrame::MainFrame(Window* pWindow) : Frame(pWindow)
	{
		s_pInstance = this;

		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		_pMainArea = CreateControl<Area>();
		_pSidePanel = CreateControl<SidePanel>();

		// Status bar
		_pStatusBar = CreateControl<StatusBar>();

		auto topSizer = SetSizer<VerticalSizer>();
		auto mainSizer = topSizer->Add<HorizontalSizer>(-1, Sizer::Expand);

		mainSizer->Add(_pSidePanel, 0, Sizer::Expand);
		mainSizer->Add(_pMainArea, -1, Sizer::Fill);

		topSizer->Add(_pStatusBar, 0, Sizer::Expand);

		RegisterScreen<LoginScreen>(ScreenType::Login);
		RegisterScreen<HomeScreen>(ScreenType::Home);
		RegisterScreen<ChatScreen>(ScreenType::Chat);
		RegisterScreen<DebugScreen>(ScreenType::Debug);
		RegisterScreen<ChatListingScreen>(ScreenType::ChatListing);

		DebugUtility::Initialize();

		// Sign in
		auto& userMngr = Global::GetUserManager();
		if (not userMngr.LoadProfiles())
			userMngr.CreateDefaultProfile();

		// Try auto sign-in
		if (not AutoSignIn())
			ShowLoginScreen();
	}

	MainFrame::~MainFrame()
	{
		for (auto& [_, pScreen] : _screensByType)
		{
			auto pParent = pScreen->GetParent();
			if (pParent)
				pParent->RemoveChild(pScreen);
			delete pScreen;
		}
		_screensByType.clear();
	}

	void MainFrame::OnUpdate(float fElapsed)
	{
		if (auto pLLM = Global::GetLLMInstance())
		{
			if (auto pSession = pLLM->GetSession().lock())
				pSession->Update(fElapsed);
		}
	}

	void MainFrame::OnRender(Renderer* pRenderer)
	{
		DrawBackground(pRenderer);
	}

	void MainFrame::SetStatusBar(fig::string_view message)
	{
		s_pInstance->_pStatusBar->SetMessage(toStr(message));
	}

	void MainFrame::SetStatusBar(const fig::llm::LLMStatus& status)
	{
		s_pInstance->_pStatusBar->SetModelInfo(status);
	}

	void MainFrame::Close()
	{
		SDL_Event quit_event;
		SDL_zero(quit_event);
		quit_event.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quit_event);
	}

	void MainFrame::ChangeScreen(ScreenType screen)
	{
		Screen* pScreen = nullptr;
		if (auto itFind = _screensByType.find(screen); itFind != _screensByType.end())
			pScreen = itFind->second;

		if (_pActiveScreen)
		{
			_pActiveScreen->SetVisible(false);
			_pActiveScreen->PushEvent(UserEvent::Deactivated);

			// Detach
			auto pParent = _pActiveScreen->GetParent();
			if (pParent)
				pParent->RemoveChild(pScreen);
			_pActiveScreen = nullptr;
		}

		_pActiveScreen = pScreen;
		if (not (bool)_pActiveScreen)
			return;

		_pMainArea->RemoveChildren();
		_pMainArea->AddChild(_pActiveScreen);
		
		auto sizer = _pMainArea->SetSizer<VerticalSizer>();
		sizer->Add(_pActiveScreen, -1, Sizer::Fill);

		_pActiveScreen->SetVisible(true);
		_pActiveScreen->PushEvent(UserEvent::Activated);
		_pActiveScreen->NotifySidePanelShown(_pSidePanel->GetVisible());

		InvalidateLayout();

		// Reset cursor
		Global::SetCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	}

	template<IsScreen T>
	void MainFrame::RegisterScreen(ScreenType screen)
	{
		if (_screensByType.contains(screen))
			UnregisterScreen(screen);

		auto pScreen = new T(this);	// Must pass this to receive renderer
		RemoveChild(pScreen);
		_screensByType[screen] = pScreen;
		pScreen->SetSize(GetSize());
	}

	void MainFrame::UnregisterScreen(ScreenType screen)
	{
		if (auto itFind = _screensByType.find(screen); itFind != _screensByType.end())
		{
			auto pScreen = itFind->second;
			_pMainArea->RemoveChild(pScreen);
			RemoveChild(pScreen);
			_screensByType.erase(screen);
			delete pScreen;
		}
	}

	void MainFrame::ShowSidePanel(bool bShow) noexcept
	{
		_pSidePanel->SetVisible(bShow);
		_pSidePanel->EnableLayout(bShow);

		if (_pActiveScreen)
			_pActiveScreen->NotifySidePanelShown(bShow);
		InvalidateLayout();
	}

	void MainFrame::ShowLoginScreen()
	{
		// Show login screen
		ChangeScreen(ScreenType::Login);
		ShowSidePanel(false);
	}

	bool MainFrame::TrySignIn(const fig::user::UserProfile& profile, const fig::string& password) noexcept
	{
		auto& userMngr = Global::GetUserManager();
		auto startTime = std::chrono::steady_clock::now();

		if (userMngr.SignIn(profile.id, password))
		{
			OnSignedIn(profile);

			auto endTime = std::chrono::steady_clock::now();
			// MainFrame::SetStatusBar(std::format("Duration: {}ms", toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count())));
			return true;
		}

		return false;
	}

	bool MainFrame::SignOut() noexcept
	{
		auto& userMngr = Global::GetUserManager();
		if (userMngr.SignOut())
		{
			OnSignedOut();
			return true;
		}
		return false;
	}

	void MainFrame::OnSignedIn(const fig::user::UserProfile& profile) noexcept
	{
		// Remember
		Global::GetSettings().SetUUID(AppSetting::LastUser, profile.id);
		Global::GetSettings().SetBool(AppSetting::SignedIn, true);

		PushEvent(UserEvent::UserSignedIn, &profile);

		ShowSidePanel(true);

		ChangeScreen(ScreenType::Home);
		auto pHomeScreen = GetScreen<HomeScreen>(ScreenType::Home);
		pHomeScreen->CreateCards();
	}

	void MainFrame::OnSignedOut() noexcept
	{
		PushEvent(UserEvent::UserSignedOut);

		ShowSidePanel(false);
		ChangeScreen(ScreenType::Login);

		Global::GetSettings().SetBool(AppSetting::SignedIn, false);
	}

	bool MainFrame::AutoSignIn() noexcept
	{
		if (not Global::GetSettings().GetBool(AppSetting::SignedIn))
			return false;

		auto& profiles = Global::GetUserManager().GetProfiles();
		if (profiles.empty())
			return false;

		if (auto lastProfile = Global::GetUserManager().GetProfile(Global::GetSettings().GetUUID(AppSetting::LastUser)))
		{
			if (not (*lastProfile).has_password)
				return TrySignIn(*lastProfile, "");
			return false;
		}

		return TrySignIn(profiles.front(), "");
	}

	void MainFrame::InitializeModel()
	{
		auto& backend = Global::GetLLMBackend();

		if (!backend.IsInitialized() && !backend.IsInitializing())
		{
			SetStatusBar(fig::strings::Status::LoadingModel);

			// Read model settings
			fig::data::ModelSettings modelSettings {};
			if (auto try_model_settings = Global::GetUserContent().GetActiveModelSettings())
				modelSettings = try_model_settings.value();
			else
			{
				PushEvent(UserEvent::LLMModelLoadFailure);
				return;
			}

			backend.Initialize(modelSettings,
				[this](int percent) {
					PushEvent(UserEvent::LLMModelLoadingProgress, percent);
				},
				[this, &backend](bool bSuccess) {
					if (bSuccess)
					{
						auto pInstance = backend.CreateInstance();
						Global::SetLLMInstance(pInstance);
					}
				});
		}
	}

	void MainFrame::UnloadModel()
	{
		auto& backend = Global::GetLLMBackend();
		if (backend.IsInitialized())
		{
			backend.Shutdown();
			SetStatusBar(fig::strings::Status::ModelUnloaded);
		}
	}

	EventResult MainFrame::OnEvent(Event& event)
	{
		if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if constexpr (Debugging)
				{
					if (keyEvent.key == SDLK_F12 and mods.Control)
					{
						Close();
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_3 and mods.Alt)
					{
						ChangeScreen(ScreenType::Debug);
						return EventResult::Handled;
					}
					else if (keyEvent.key == SDLK_F12 and mods.Control)
					{
						Close();
						return EventResult::Handled;
					}
				}

				if (keyEvent.key == SDLK_1 and mods.Alt)
				{
					ChangeScreen(ScreenType::Home);
					return EventResult::Handled;
				}
				else if (keyEvent.key == SDLK_2 and mods.Alt)
				{
					ChangeScreen(ScreenType::Chat);
					return EventResult::Handled;
				}
				else if (keyEvent.key == SDLK_TAB and mods.None)
				{
					ShowSidePanel(!_pSidePanel->GetVisible());
					return EventResult::Handled;
				}
				else if (keyEvent.key == SDLK_F2 and mods.None)
				{
					InitializeModel();
					return EventResult::Handled;
				}
				else if (keyEvent.key == SDLK_F3 and mods.None)
				{
					UnloadModel();
					return EventResult::Handled;
				}
			}
		}

		if (IsUserEventWithData(event, UserEvent::LLMStatusUpdate))
		{
			SetStatusBar(GetUserData<fig::llm::LLMStatus>(event));
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMChatInitializing))
		{
			SetStatusBar(fig::strings::Status::InitializingChat);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMChatInitialized))
		{
			SetStatusBar(fig::strings::Status::ChatInitialized);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMChatInitializationFailure))
		{
			SetStatusBar(fig::strings::Status::FailedToInitializeChat);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoading))
		{
			SetStatusBar(fig::strings::Status::LoadingModel);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoaded))
		{
			SetStatusBar(fig::strings::Status::ModelLoaded);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelUnloaded))
		{
			SetStatusBar(fig::strings::Status::ModelUnloaded);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelLoadFailure))
		{
			SetStatusBar(fig::strings::Status::FailedToLoadModel);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMGenerationStarted))
		{
			SetStatusBar(fig::strings::Status::GeneratingResponse);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMRebuildingKVCache))
		{
			SetStatusBar(fig::strings::Status::RebuildingContext);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMGenerationComplete))
		{
			SetStatusBar(fig::strings::Status::Ready);
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::LLMModelUnloadRequest))
		{
			UnloadModel();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::StartChat))
		{
			const fig::uuid& characterId = GetUserData<fig::uuid>(event);
			StartChat(characterId);
			return EventResult::Handled;
		}
		else if (IsUserEvent(event, UserEvent::Scrolling))
		{
			PopAllMenus();
			return EventResult::Continue;
		}

		if constexpr (Debugging)
		{
			if (IsUserEvent(event, UserEvent::DebugCharacter))
			{
				const fig::uuid& characterId = GetUserData<fig::uuid>(event);
				DebugUtility::DebugCharacter(characterId);
				return EventResult::Handled;
			}
		}

		if (_pActiveScreen)
			return _pActiveScreen->ProcessEvent(event);
		return EventResult::Pass;
	}

	bool MainFrame::StartChat(const fig::uuid& characterId)
	{
		ChangeScreen(ScreenType::Chat);
		auto pChatScreen = GetScreen<ChatScreen>(ScreenType::Chat);

		if (auto character = Global::GetUserManager().GetContent().Get<Character>(characterId))
		{
			PromptScaffold scaffold;
			if (!Success(scaffold.LoadFromXml(fig::path(Constants::Paths::PromptScaffold))))
				return false;

			Scenario scenario;
			if (!Success(scenario.LoadFromXml(fig::path(Constants::Paths::DefaultScenario))))
				return false;

			ChatStaging staging(std::move(scenario), std::move(scaffold), Constants::LLM::DefaultChatOptions);

			if (!staging.AddCharacter(characterId, Role::Bot1, character.value()))
				return false;

			Character user;
			if (not (Success(user.LoadFromXml(fig::path { "./characters/user.xml" })) and staging.AddCharacter({}, Role::User, user))) //! @temp
				return false;

			// Create instance
			ChatInstance instance;
			instance.characterIds = staging.GetCharacterIds();
			instance.userId = {}; //! @todo
			instance.scenarioId = {}; //! @todo
			instance.options = Constants::LLM::DefaultChatOptions; //! @todo
			
			auto& chatInstanceAsset = Global::GetUserManager().GetContent().CreateAsset(instance);

			pChatScreen->StartChat(staging, chatInstanceAsset.id);
			return true;
		}
		return false;
	}

}