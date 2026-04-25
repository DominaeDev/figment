#include <pch.h>
#include "gui/MainFrame.h"
#include "gui/HomeScreen.h"
#include "gui/ChatScreen.h"
#include "gui/DebugScreen.h"
#include "gui/SidePanel.h"
#include "gui/LoginScreen.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "fs/FileUtility.h"

namespace fig::gui
{
	MainFrame* MainFrame::s_pInstance = nullptr;

	MainFrame::MainFrame(Window* pWindow) : Frame(pWindow)
	{
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
		s_pInstance = this;

		RegisterScreen<LoginScreen>(ScreenType::Login);
		RegisterScreen<HomeScreen>(ScreenType::Home);
		RegisterScreen<ChatScreen>(ScreenType::Chat);
		RegisterScreen<DebugScreen>(ScreenType::Debug);

		// Sign in
		auto& userMngr = Global::GetUserManager();
		if (not userMngr.LoadProfiles())
			userMngr.CreateDefaultProfile();

		// Import test scenario
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& content = userMngr.GetContent();

				auto remove_scenarios = content.GetAssetManager().GetScenarioAssets()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				content.GetAssetManager().DeleteAssets(remove_scenarios);

				auto _ignored = content.ImportScenario(fig::path("./import/scenario.xml"));
				userMngr.SignOut();
			}
		}

		// Import test characters
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& content = userMngr.GetContent();

				// Delete all characters
				auto remove_characters = content.GetAssetManager().GetCharacterAssets()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				content.GetAssetManager().DeleteAssets(remove_characters);

				auto _ignored = content.ImportCharactersInDirectory(fig::path("./import/characters"));
				userMngr.SignOut();
			}
		}

		// Shuffle cards
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& content = userMngr.GetContent();

				auto characterAssets = content.GetAssetManager().GetAssets() 
					| std::views::filter([](auto& a) { return a.asset_type == fig::io::AssetType::Character; })
					| std::views::transform([](auto& a) { return std::ref(a); })
					| std::ranges::to<std::vector>();

				auto rng = std::random_device {};
				std::ranges::shuffle(characterAssets, rng);

				int32_t count = 0;
				auto timestamp = fig::util::utc_now();
				for (auto& assetRef : characterAssets)
				{
					auto& asset = assetRef.get();
					content.MarkNew(asset.id, count++ < 10);
					asset.SetMeta(fig::io::MetaTag::CreatedAt, timestamp);
					asset.SetMeta(fig::io::MetaTag::LastUsedAt, timestamp);
					asset.SetMeta(fig::io::MetaTag::UpdatedAt, timestamp);
					timestamp -= static_cast<fig::timestamp>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(100)).count());
				}

				userMngr.SignOut();
			}
		}

		// Create profile pic
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& assetMngr = userMngr.GetContent().GetAssetManager();
				assetMngr.CreateProfilePicture(userMngr.GetActiveProfile(), fig::path("./import/profile_pic.png"));
				userMngr.SignOut();
			}
		}

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
			}
		}

		if (event.type == USER_EVENT(EventType::LLMStatusUpdate))
			SetStatusBar(*reinterpret_cast<fig::llm::LLMStatus*>(event.user.data1));
		if (event.type == USER_EVENT(EventType::LLMChatInitializing))
			SetStatusBar(fig::strings::Status::InitializingChat);
		else if (event.type == USER_EVENT(EventType::LLMChatInitialized))
			SetStatusBar(fig::strings::Status::ChatInitialized);
		else if (event.type == USER_EVENT(EventType::LLMChatInitializationFailure))
			SetStatusBar(fig::strings::Status::FailedToInitializeChat);
		else if (event.type == USER_EVENT(EventType::LLMModelLoading))
			SetStatusBar(fig::strings::Status::LoadingModel);
		else if (event.type == USER_EVENT(EventType::LLMModelLoaded))
			SetStatusBar(fig::strings::Status::ModelLoaded);
		else if (event.type == USER_EVENT(EventType::LLMModelUnloaded))
			SetStatusBar(fig::strings::Status::ModelUnloaded);
		else if (event.type == USER_EVENT(EventType::LLMModelLoadFailure))
			SetStatusBar(fig::strings::Status::FailedToLoadModel);
		else if (event.type == USER_EVENT(EventType::LLMGenerationStarted))
			SetStatusBar(fig::strings::Status::GeneratingResponse);
		else if (event.type == USER_EVENT(EventType::LLMRebuildingKVCache))
			SetStatusBar(fig::strings::Status::RebuildingContext);
		else if (event.type == USER_EVENT(EventType::LLMGenerationComplete))
			SetStatusBar(fig::strings::Status::Ready);

		if (_pActiveScreen)
			return _pActiveScreen->ProcessEvent(event);
		return false;
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
}