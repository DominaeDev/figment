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
		mainSizer->Add(_pMainArea, -1, Sizer::Expand);

		auto topSizer = new VerticalSizer();
		topSizer->AddSizer(mainSizer, -1, Sizer::Expand);
		topSizer->Add(_pStatusBar, 0, Sizer::Expand);

		SetSizer(topSizer);
		s_pInstance = this;

		RegisterScreen<LoginScreen>();
		RegisterScreen<HomeScreen>();
		RegisterScreen<ChatScreen>();
		RegisterScreen<DebugScreen>();

		// Sign in
		auto& userMngr = Global::GetUserManager();
		if (not userMngr.LoadProfiles())
			userMngr.CreateDefaultProfile();

		// Import test characters
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& assets = userMngr.GetProfileAssets();

				// Delete all characters
				auto remove_characters = assets.GetAllCharacters()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				assets.DeleteAssets(remove_characters);

				auto x = assets.ImportCharacter(fig::path("./import/user.xml"));
				auto y = assets.ImportCharacter(fig::path("./import/bot1.xml"));
				auto z = assets.ImportCharacter(fig::path("./import/bot2.xml"));
				userMngr.GetProfileAssets().SaveModified();
			}
		}

		// Import test scenario
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& assets = userMngr.GetProfileAssets();
				auto remove_scenarios = assets.GetAllScenarios()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				assets.DeleteAssets(remove_scenarios);

				auto x = assets.ImportScenario(fig::path("./import/scenario.xml"));
				userMngr.GetProfileAssets().SaveModified();
			}
		}

		// Import test characters
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& assets = userMngr.GetProfileAssets();

				// Delete all characters
				auto remove_characters = assets.GetAllCharacters()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				assets.DeleteAssets(remove_characters);

				assets.ImportCharactersInDirectory(fig::path("./import/characters"), fig::io::AssetManager::CharacterDataFormat::TavernV2);
				userMngr.GetProfileAssets().SaveModified();
				userMngr.SignOut();
			}
		}

		// Create profile pic
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.SignInDefaultProfile())
			{
				auto& assets = userMngr.GetProfileAssets();
				assets.CreateProfilePicture(userMngr.GetActiveProfile(), fig::path("./import/profile_pic.png"));
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

			if (keyEvent.down and not keyEvent.repeat)
			{
#if _DEBUG
				if (keyEvent.key == SDLK_F12 and ((keyEvent.mod & SDL_KMOD_CTRL) != 0))
				{
					Close();
					return true;
				}
#endif

				if (keyEvent.key == SDLK_1 and ((keyEvent.mod & SDL_KMOD_ALT) != 0))
				{
					ChangeScreen<HomeScreen>();
					return true;
				}
				else if (keyEvent.key == SDLK_2 and ((keyEvent.mod & SDL_KMOD_ALT) != 0))
				{
					ChangeScreen<ChatScreen>();
					return true;
				}
				else if (keyEvent.key == SDLK_TAB and ((keyEvent.mod & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT)) == 0))
				{
					ShowSidePanel(!_pSidePanel->GetVisible());
					return true;
				}
#if _DEBUG
				else if (keyEvent.key == SDLK_3 and ((keyEvent.mod & SDL_KMOD_ALT) != 0))
				{
					ChangeScreen<DebugScreen>();
					return true;
				}
#endif
			}
		}

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

	void MainFrame::ChangeScreen(Screen* pScreen)
	{
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

		_pMainArea->AddChild(_pActiveScreen);
		
		auto sizer = new VerticalSizer();
		sizer->Add(_pActiveScreen, -1, Sizer::Expand);
		_pMainArea->SetSizer(sizer);

		_pActiveScreen->SetVisible(true);
		_pActiveScreen->NotifySidePanelShown(_pSidePanel->GetVisible());

		InvalidateLayout();

		// Reset cursor
		Global::SetCursor(SDL_SYSTEM_CURSOR_DEFAULT);
	}

	template<IsScreen T>
	void MainFrame::RegisterScreen()
	{
		if (_screensByType.contains(type_id<T>()))
			UnregisterScreen<T>();

		auto pScreen = new T(this);	// Must pass this to receive renderer
		RemoveChild(pScreen);
		_screensByType[type_id<T>()] = pScreen;
		pScreen->SetSize(GetSize());
	}

	template<IsScreen T>
	void MainFrame::UnregisterScreen()
	{
		auto pScreen = GetScreen<T>();
		if (!pScreen)
			return;

		_pMainArea->RemoveChild(pScreen);
		RemoveChild(pScreen);
		_screensByType.erase(type_id<T>());
		delete pScreen;
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
		ChangeScreen<LoginScreen>();
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

		if (_pSidePanel)
			_pSidePanel->SetUserProfile(profile);
		ShowSidePanel(true);

		auto pHomeScreen = ChangeScreen<HomeScreen>();
		pHomeScreen->CreateCards();
	}

	void MainFrame::OnSignedOut() noexcept
	{
		if (_pSidePanel)
			_pSidePanel->Reset();

		ShowSidePanel(false);
		ChangeScreen<LoginScreen>();
	}

	bool MainFrame::AutoSignIn() noexcept
	{
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