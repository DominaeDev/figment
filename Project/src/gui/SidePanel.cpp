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

		_pExpandedRoot = CreateControl<Area>();
		_pCollapsedRoot = CreateControl<Area>();

		auto pHeaderPanel = _pExpandedRoot->CreateControl<Panel>();
		pHeaderPanel->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pLogo = pHeaderPanel->CreateControl<Image>(AppResources::GetTexture(TextureType::LOGO_SMALL), Colors::Black);
		pLogo->SetX(44);

		auto pGradient = _pExpandedRoot->CreateControl<HorizontalGradient>(Colors::SidePanelGradient.WithAlpha(0.0f), Colors::SidePanelGradient.WithAlpha(0.8f));
		_pGradient = pGradient;

		_pMenuButton = pHeaderPanel->CreateControl<ButtonWithIcon>(TextureType::ICON_MENU);
		_pMenuButton->SetTheme(Themes::SidePanelButtonStyle);
		_pMenuButton->SetX(4);
		_pMenuButton->CenterVertically();
		_pMenuButton->SetDelegate([this]() { ShowMenu(); });

		_pCollapseButton = CreateControl<ButtonWithIcon>(TextureType::ICON_EXPAND_ARROW_LEFT);
		_pCollapseButton->SetTheme(Themes::SidePanelButtonStyle);
		_pCollapseButton->SetSize(36, 36);
		_pCollapseButton->SetX(GetWidth() - _pCollapseButton->GetWidth() - 4);
		_pCollapseButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pCollapseButton->GetHeight()) / 2);
		_pCollapseButton->SetDelegate([this]() { 
			_bExpanded ? Collapse() : Expand();
		});

		auto pMainArea = _pExpandedRoot->CreateControl<Area>();

		auto pChatButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_CHATS, toStr(fig::strings::UI::MenuRecentChats));
		pChatButton->SetDelegate([]() { PushEvent(UserEvent::NavigateToChatList); });
		auto pCharactersButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_CHARACTERS, toStr(fig::strings::UI::MenuCharacters));
		pCharactersButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen(ScreenType::Home); });
		auto pScenariosButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_SCENARIOS, toStr(fig::strings::UI::MenuScenarios));
		auto pWorldsButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_WORLDS, toStr(fig::strings::UI::MenuWorlds));
		auto pModelsButton = pMainArea->CreateControl<SidePanelButton>(TextureType::ICON_MENU_MODELS, "Models");

		auto pChatButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(TextureType::ICON_MENU_CHATS_SMALL);
		pChatButtonSmall->SetDelegate([]() { PushEvent(UserEvent::NavigateToChatList); });
		pChatButtonSmall->SetTheme(Themes::SidePanelButtonStyle);
		auto pCharactersButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(TextureType::ICON_MENU_CHARACTERS_SMALL);
		pCharactersButtonSmall->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen(ScreenType::Home); });
		pCharactersButtonSmall->SetTheme(Themes::SidePanelButtonStyle);
		auto pScenariosButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(TextureType::ICON_MENU_SCENARIOS_SMALL);
		pScenariosButtonSmall->SetTheme(Themes::SidePanelButtonStyle);
		auto pWorldsButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(TextureType::ICON_MENU_WORLDS_SMALL);
		pWorldsButtonSmall->SetTheme(Themes::SidePanelButtonStyle);
		auto pModelsButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(TextureType::ICON_MENU_MODELS_SMALL);
		pModelsButtonSmall->SetTheme(Themes::SidePanelButtonStyle);

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

		auto pFooterPanel = _pExpandedRoot->CreateControl<Area>();
		pFooterPanel->SetHeight(Constants::GUI::SidePanel::FooterHeight);

		_pModelWidget = pFooterPanel->CreateControl<LoadModelWidget>();
		_pModelWidget->SetHeight(60);
		_pModelWidget->SetMaxSize(-1, 60);

		_pUserWidget = pFooterPanel->CreateControl<UserProfileWidget>();
		_pUserWidget->SetHeight(60);
		
		auto pMainSizer =_pExpandedRoot->SetSizer<VerticalSizer>();
		pMainSizer->Add(pHeaderPanel, 0, Sizer::Expand | Sizer::Fill);
		pMainSizer->Add(pMainArea, -1, Sizer::Fill);
		pMainSizer->Add(_pModelWidget, 0, Sizer::Expand);
		pMainSizer->Add(_pUserWidget, 0, Sizer::Expand);

		auto pSmallButtonSizer = _pCollapsedRoot->SetSizer<VerticalSizer>();
		pSmallButtonSizer->AddSpacer(56);
		pSmallButtonSizer->Add(pChatButtonSmall, 0, Sizer::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pCharactersButtonSmall, 0, Sizer::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pScenariosButtonSmall, 0, Sizer::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pWorldsButtonSmall, 0, Sizer::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pModelsButtonSmall, 0, Sizer::AlignCenterHorizontal);
//		_pCollapsedRoot->Cull(true);

		SetBorderRenderer<LineBorderRenderer>(Colors::LineColor, Direction::East);
		
		_bExpanded = false;
		Expand();
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
		auto& createMenu = menu.AddItem("Create");
		createMenu.AddItem("New character\u2026");
		createMenu.AddItem("New story\u2026");
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
		_pCollapseButton->SetIcon(TextureType::ICON_EXPAND_ARROW_LEFT);
		_pExpandedRoot->Cull(false);
		_pCollapsedRoot->Cull(true);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pExpandedRoot, -1, Sizer::Expand | Sizer::Fill);

		PushEvent(UserEvent::SidePanelExpanded);
	}

	void SidePanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		SetWidth(42);
		_pCollapseButton->CenterHorizontally();
		_pCollapseButton->SetIcon(TextureType::ICON_EXPAND_ARROW_RIGHT);
		_pExpandedRoot->Cull(true);
		_pCollapsedRoot->Cull(false);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pCollapsedRoot, -1, Sizer::Expand | Sizer::Fill);

		PushEvent(UserEvent::SidePanelCollapsed);
	}
}