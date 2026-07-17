#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"
#include "gui/SidePanelButton.h"
#include "gui/UserProfileWidget.h"
#include "gui/LoadModelWidget.h"
#include "gui/LineBorderRenderer.h"
#include "gui/Menu.h"

namespace fig::gui
{
	SidePanel::SidePanel(ParentPtr pParent) : Control(pParent)
	{
		SetWidth(Constants::GUI::SidePanel::Width);
		SetBackgroundColor(fig::gui::Colors::SidePanelBackground);

		_pRootPanel = CreateControl<Area>();

		auto pHeaderPanel = _pRootPanel->CreateControl<Panel>();
		pHeaderPanel->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pLogo = pHeaderPanel->CreateControl<Image>(AppResources::GetTexture(TextureType::LOGO_SMALL), Colors::Black);
		pLogo->SetX(44);

		auto pGradient = _pRootPanel->CreateControl<HorizontalGradient>(Colors::SidePanelGradient.WithAlpha(0.0f), Colors::SidePanelGradient.WithAlpha(0.8f));
		_pGradient = pGradient;

		_pMenuButton = pHeaderPanel->CreateControl<ButtonWithIcon>(TextureType::ICON_MENU);
		_pMenuButton->SetTheme(Themes::SidePanelButtonStyle);
		_pMenuButton->SetSize(36, 36);
		_pMenuButton->SetX(4);
		_pMenuButton->CenterVertically();
		_pMenuButton->SetDelegate([this]() { ShowMenu(); });

		_pCollapseButton = CreateControl<ButtonWithIcon>(TextureType::ICON_SIDEBAR);
		_pCollapseButton->SetTheme(Themes::SidePanelButtonStyle);
		_pCollapseButton->SetSize(36, 36);
		_pCollapseButton->SetX(GetWidth() - _pCollapseButton->GetWidth() - 4);
		_pCollapseButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pCollapseButton->GetHeight()) / 2);
		_pCollapseButton->SetDelegate([]() { PushEvent(UserEvent::ToggleSidePanel); });

		auto pMainArea = _pRootPanel->CreateControl<Area>();

		auto pChatButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_CHATS, toStr(fig::strings::UI::MenuRecentChats));
		pChatButton->SetDelegate([]() { 
			PushEvent(UserEvent::NavigateToChatList);
		});
		auto pCharactersButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_CHARACTERS, toStr(fig::strings::UI::MenuCharacters));
		pCharactersButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen(ScreenType::Home); });
		auto pScenariosButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_SCENARIOS, toStr(fig::strings::UI::MenuScenarios));
		auto pWorldsButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_WORLDS, toStr(fig::strings::UI::MenuWorlds));
		auto pModelsButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_MODELS, "Models");

		auto pButtonSizer = pMainArea->SetSizer<VerticalSizer>();

		pButtonSizer->AddSpacer(20);
		pButtonSizer->Add(pChatButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pCharactersButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pScenariosButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pWorldsButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pModelsButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);

		auto pFooterPanel = _pRootPanel->CreateControl<Area>();
		pFooterPanel->SetHeight(Constants::GUI::SidePanel::FooterHeight);

		_pModelWidget = pFooterPanel->CreateControl<LoadModelWidget>();
		_pModelWidget->SetHeight(60);
		_pModelWidget->SetMaxSize(-1, 60);

		_pUserWidget = pFooterPanel->CreateControl<UserProfileWidget>();
		_pUserWidget->SetHeight(60);
		
		auto pMainSizer =_pRootPanel->SetSizer<VerticalSizer>();
		pMainSizer->Add(pHeaderPanel, 0, Sizer::Expand | Sizer::Fill);
		pMainSizer->Add(pMainArea, -1, Sizer::Fill);
		pMainSizer->Add(_pModelWidget, 0, Sizer::Expand);
		pMainSizer->Add(_pUserWidget, 0, Sizer::Expand);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pRootPanel, -1, Sizer::Expand | Sizer::Fill);

		SetBorderRenderer<LineBorderRenderer>(Colors::LineColor, Direction::East);
	}

	void SidePanel::OnAfterLayout()
	{
		constexpr Coord kGradientSize = 8;
		_pGradient->SetX(GetWidth() - kGradientSize);
		_pGradient->SetSize(kGradientSize, GetHeight());
	}

	void SidePanel::ShowMenu()
	{
		auto& menu = MainFrame::GetInstance().CreateMenu();
		menu.AddItem("New chat\u2026");
		auto& createMenu = menu.AddItem("Create");
		createMenu.AddItem("New character\u2026");
		createMenu.AddItem("New scenario\u2026");
		createMenu.AddItem("New world\u2026");
		createMenu.AddSeparator();
		createMenu.AddItem("From file\u2026")
			.SetEnabled(false);
		menu.AddSeparator();
		menu.AddItem("User profile\u2026", TextureType::ICON_USER_SETTINGS);
		menu.AddItem("Settings\u2026", TextureType::ICON_SETTINGS);
		menu.AddSeparator();
		menu.AddItem("Sign out", TextureType::ICON_LOGOUT)
			.SetDelegate([]() { MainFrame::GetInstance().SignOut(); });
		menu.AddItem("Exit Figment")
			.SetDelegate([]() { MainFrame::GetInstance().Close(); });

		menu.Show(Point { _pMenuButton->GetX(), _pMenuButton->GetY() + _pMenuButton->GetHeight() });
	}

	EventResult SidePanel::OnEvent(Event& event)
	{
		if (IsUserEvent(event, UserEvent::UserSignedIn))
		{
			_pUserWidget->SetUser(GetUserData<fig::user::UserProfile>(event));
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::UserSignedOut))
		{
			_pUserWidget->Reset();
			return EventResult::Continue;
		}
		else if (IsUserEvent(event, UserEvent::ToggleSidePanel))
		{
			_bExpanded ? Collapse() : Expand();
			return EventResult::Continue;
		}

		if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
		{
			SDL_KeyboardEvent& keyEvent = event.key;
			KeyboardMods mods { event };

			if (keyEvent.down and not keyEvent.repeat)
			{
				if (keyEvent.key == SDLK_TAB and mods.None)
				{
					_bExpanded ? Collapse() : Expand();
					return EventResult::Handled;
				}
			}
		}

		return EventResult::Pass;
	}

	void SidePanel::Expand() noexcept
	{
		if (_bExpanded)
			return;
		_bExpanded = true;

		SetWidth(Constants::GUI::SidePanel::Width);
		_pCollapseButton->SetX(GetWidth() - _pCollapseButton->GetWidth() - 4);
		_pRootPanel->Cull(false);

		PushEvent(UserEvent::SidePanelExpanded);
	}

	void SidePanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		SetWidth(42);
		_pCollapseButton->CenterHorizontally();
		_pRootPanel->Cull(true);

		PushEvent(UserEvent::SidePanelCollapsed);
	}
}