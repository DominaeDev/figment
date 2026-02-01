#include <pch.h>
#include "gui/MainFrame.h"
#include "gui/HomeFrame.h"
#include "gui/ChatFrame.h"
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

		// Status bar
		_pStatusBar = new StatusBar(this);

		auto topSizer = new VerticalSizer();
		topSizer->Add(_pMainArea, -1, Sizer::Expand);
		topSizer->Add(_pStatusBar, 0, Sizer::Expand);

		SetSizer(topSizer);
		s_pInstance = this;

		_pHomeFrame = new HomeFrame(this);
		_pChatFrame = new ChatFrame(this);
		ChangeScreen(ScreenType::Chat);

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

		if constexpr (Disabled)
		{
			// Import test characters
			if (userMngr.IsSignedIn())
			{
				auto& assets = userMngr.GetProfileAssets();
				auto x = assets.ImportCharacter(fig::path("./characters/user.xml"));
				auto y = assets.ImportCharacter(fig::path("./characters/bot1.xml"));
				auto z = assets.ImportCharacter(fig::path("./characters/bot2.xml"));
				assets.SaveModified();
			}
		}
	}

	MainFrame::~MainFrame()
	{
		if (_pActiveScreen)
		{
			_pMainArea->RemoveChild(_pActiveScreen);
			_pActiveScreen = nullptr;
		}
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
				switch (keyEvent.key)
				{
#if _DEBUG
				case SDLK_F12:
					if (keyEvent.mod == SDL_KMOD_CTRL)
					{
						Close();
						return true;
					}
					break;
#endif
				}
			}
		}
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
		if (_pActiveScreen)
		{
			_pActiveScreen->SetVisible(false);
			_pActiveScreen = nullptr;
		}

		switch (screen)
		{
		case ScreenType::Home:
			_pActiveScreen = _pHomeFrame;
			break;
		case ScreenType::Chat:
			_pActiveScreen = _pChatFrame;
			break;
		}

		if (not (bool)_pActiveScreen)
			return;

		_pActiveScreen->SetVisible(true);

		_pMainArea->RemoveChildren();
		_pMainArea->AddChild(_pActiveScreen);
		
		auto sizer = new VerticalSizer();
		sizer->Add(_pActiveScreen, -1, Sizer::Expand);
		_pMainArea->SetSizer(sizer);
	}
}