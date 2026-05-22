#include <pch.h>
#include "gui/MainFrame.h"
#include "gui/HomeScreen.h"
#include "gui/ChatScreen.h"
#include "gui/DebugScreen.h"
#include "gui/SidePanel.h"
#include "gui/LoginScreen.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/FileUtility.h"
#include "llm/LLMBackend.h"
#include "llm/LLMInstance.h"
#include "llm/LLMUtility.h"
#include "data/ModelSettings.h"
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

		_pMainArea = new Area(this);
		_pSidePanel = new SidePanel(this);

		// Status bar
		_pStatusBar = new StatusBar(this);

		auto mainSizer = new HorizontalSizer();
		mainSizer->Add(_pSidePanel, 0, Sizer::Expand);
		mainSizer->Add(_pMainArea, -1, Sizer::Fill);

		auto topSizer = new VerticalSizer();
		topSizer->Add(mainSizer, -1, Sizer::Expand);
		topSizer->Add(_pStatusBar, 0, Sizer::Expand);

		SetSizer(topSizer);

		RegisterScreen<LoginScreen>(ScreenType::Login);
		RegisterScreen<HomeScreen>(ScreenType::Home);
		RegisterScreen<ChatScreen>(ScreenType::Chat);
		RegisterScreen<DebugScreen>(ScreenType::Debug);

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
		
		auto sizer = new VerticalSizer();
		sizer->Add(_pActiveScreen, -1, Sizer::Fill);
		_pMainArea->SetSizer(sizer);

		_pActiveScreen->SetVisible(true);
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

		PushEvent(EventType::UserSignedIn, &profile);

		ShowSidePanel(true);

		ChangeScreen(ScreenType::Home);
		auto pHomeScreen = GetScreen<HomeScreen>(ScreenType::Home);
		pHomeScreen->CreateCards();
	}

	void MainFrame::OnSignedOut() noexcept
	{
		PushEvent(EventType::UserSignedOut);

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
			if (not lastProfile.value().get().has_password)
				return TrySignIn(lastProfile.value(), "");
			return false;
		}

		return TrySignIn(profiles.front(), "");
	}

	void MainFrame::InitializeModel()
	{
		auto& engine = Global::GetLLMEngine();

		if (!engine.IsInitialized() && !engine.IsInitializing())
		{
			SetStatusBar(fig::strings::Status::LoadingModel);

			// Read model settings
			fig::data::ModelSettings modelSettings {};
			if (auto try_model_settings = Global::GetUserContent().GetActiveModelSettings())
				modelSettings = try_model_settings.value();
			else
			{
				PushEvent(EventType::LLMModelLoadFailure);
				return;
			}

			engine.Initialize(modelSettings,
				[this](int percent) {
					PushEvent(EventType::LLMModelLoadingProgress, percent);
				},
				[this, &engine](bool bSuccess) {
					if (bSuccess)
					{
						auto pInstance = engine.CreateInstance();
						Global::SetLLMInstance(pInstance);
					}
				});
		}
	}

	void MainFrame::UnloadModel()
	{
		auto& engine = Global::GetLLMEngine();
		if (engine.IsInitialized())
		{
			engine.Shutdown();
			SetStatusBar(fig::strings::Status::ModelUnloaded);
		}
	}

	bool MainFrame::OnEvent(SDL_Event& event)
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
						return true;
					}
					else if (keyEvent.key == SDLK_3 and mods.Alt)
					{
						ChangeScreen(ScreenType::Debug);
						return true;
					}
					else if (keyEvent.key == SDLK_F12 and mods.Control)
					{
						Close();
						return true;
					}
				}

				if (keyEvent.key == SDLK_1 and mods.Alt)
				{
					ChangeScreen(ScreenType::Home);
					return true;
				}
				else if (keyEvent.key == SDLK_2 and mods.Alt)
				{
					ChangeScreen(ScreenType::Chat);
					return true;
				}
				else if (keyEvent.key == SDLK_TAB and mods.None)
				{
					ShowSidePanel(!_pSidePanel->GetVisible());
					return true;
				}
				else if (keyEvent.key == SDLK_F2 and mods.None)
				{
					InitializeModel();
					return true;
				}
				else if (keyEvent.key == SDLK_F3 and mods.None)
				{
					UnloadModel();
					return true;
				}
			}
		}

		if (SDLUserEvent(event, EventType::LLMStatusUpdate))
		{
			if (event.user.data1)
				SetStatusBar(*reinterpret_cast<fig::llm::LLMStatus*>(event.user.data1));
		}
		else if (SDLUserEvent(event, EventType::LLMChatInitializing))
			SetStatusBar(fig::strings::Status::InitializingChat);
		else if (SDLUserEvent(event, EventType::LLMChatInitialized))
			SetStatusBar(fig::strings::Status::ChatInitialized);
		else if (SDLUserEvent(event, EventType::LLMChatInitializationFailure))
			SetStatusBar(fig::strings::Status::FailedToInitializeChat);
		else if (SDLUserEvent(event, EventType::LLMModelLoading))
			SetStatusBar(fig::strings::Status::LoadingModel);
		else if (SDLUserEvent(event, EventType::LLMModelLoaded))
			SetStatusBar(fig::strings::Status::ModelLoaded);
		else if (SDLUserEvent(event, EventType::LLMModelUnloaded))
			SetStatusBar(fig::strings::Status::ModelUnloaded);
		else if (SDLUserEvent(event, EventType::LLMModelLoadFailure))
			SetStatusBar(fig::strings::Status::FailedToLoadModel);
		else if (SDLUserEvent(event, EventType::LLMGenerationStarted))
			SetStatusBar(fig::strings::Status::GeneratingResponse);
		else if (SDLUserEvent(event, EventType::LLMRebuildingKVCache))
			SetStatusBar(fig::strings::Status::RebuildingContext);
		else if (SDLUserEvent(event, EventType::LLMGenerationComplete))
			SetStatusBar(fig::strings::Status::Ready);
		else if (SDLUserEvent(event, EventType::LLMModelUnloadRequest))
			UnloadModel();

		if (SDLUserEvent(event, EventType::StartChat))
		{
			const fig::uuid& characterId = *reinterpret_cast<fig::uuid*>(event.user.data1);
			StartChat(characterId);
		}

		if (_pActiveScreen)
			return _pActiveScreen->ProcessEvent(event);
		return false;
	}

	void MainFrame::StartChat(const fig::uuid& characterId)
	{
		ChangeScreen(ScreenType::Chat);
		auto pChatScreen = GetScreen<ChatScreen>(ScreenType::Chat);

		if (auto character = Global::GetUserManager().GetContent().GetCharacter(characterId))
		{
			CharacterData user;
			if (user.LoadFromXml(fig::path { "./characters/user.xml" }) != FileError::NoError) //! @temp
				return;

			ChatStaging staging(Constants::LLM::DefaultChatOptions);
			staging.AddCharacter(fig::CreateUUID(), Role::User, user); //! @temp
			staging.AddCharacter(characterId, Role::Bot1, character.value());

			pChatScreen->StartChat(staging);
		}
	}

}