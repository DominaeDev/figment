#include <pch.h>
#include "gui/MainFrame.h"
#include "gui/HomeFrame.h"
#include "gui/ChatFrame.h"
#include "gui/SidePanel.h"
#include "model/AppState.h"
#include "model/UserManager.h"

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
		topSizer->Add(mainSizer, this, -1, Sizer::Expand);
		topSizer->Add(_pStatusBar, 0, Sizer::Expand);

		SetSizer(topSizer);
		s_pInstance = this;

		RegisterScreen<HomeFrame>();
		RegisterScreen<ChatFrame>();

		// Sign in
		auto& userMngr = ApplicationState::GetUserManager();
		if (not userMngr.LoadProfiles())
		{
			userMngr.CreateDefaultProfile();
			userMngr.SaveProfiles();
			userMngr.SignInDefaultProfile();
		}
		else
		{
			userMngr.SignInDefaultProfile();
		}

		// Import test characters
		if constexpr (Debugging and Disabled)
		{
			if (userMngr.IsSignedIn())
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
			if (userMngr.IsSignedIn())
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
			if (userMngr.IsSignedIn())
			{
				auto& assets = userMngr.GetProfileAssets();

				// Delete all characters
				auto remove_characters = assets.GetAllCharacters()
					| std::views::transform([](auto& a) -> fig::uuid { return a.id; })
					| std::ranges::to<std::vector>();
				assets.DeleteAssets(remove_characters);

				assets.ImportTestCharacters(fig::path("./import/characters"));
				userMngr.GetProfileAssets().SaveModified();
			}
		}

		// Show home screen
		if (userMngr.IsSignedIn())
		{
			auto pScreen = ChangeScreen<HomeFrame>();
			pScreen->CreateCards();
		}

		userMngr.GetProfileAssets().SaveModified();
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

	void MainFrame::OnUpdate(float fDeltaTime)
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
					ChangeScreen<HomeFrame>();
					return true;
				}
				else if (keyEvent.key == SDLK_2 and ((keyEvent.mod & SDL_KMOD_ALT) != 0))
				{
					ChangeScreen<ChatFrame>();
					return true;
				}
				else if (keyEvent.key == SDLK_3 and ((keyEvent.mod & SDL_KMOD_ALT) != 0))
				{
					ShowSidePanel(not _pSidePanel->GetVisible());
					return true;
				}
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

		LayoutNow();
	}

	template<IsScreen T>
	void MainFrame::RegisterScreen()
	{
		if (_screensByType.contains(type_id<T>()))
			UnregisterScreen<T>();

		auto pScreen = new T(this);	// Must pass this to receive renderer
		RemoveChild(pScreen);
		_screensByType[type_id<T>()] = pScreen;
		pScreen->SetSize(this->_size);
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
		InvalidateLayout();
	}
}