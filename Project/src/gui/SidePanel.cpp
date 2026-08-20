#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"
#include "gui/SidePanelButton.h"
#include "gui/UserProfileWidget.h"
#include "gui/LoadModelWidget.h"
#include "gui/LineBorderRenderer.h"
#include "gui/ResizeHandle.h"
#include "gui/Menu.h"

using namespace fig::io;

namespace fig::gui
{
	SidePanel::SidePanel(ControlPtr pParent) : Control(pParent)
	{
		SetWidth(Constants::GUI::SidePanel::Width);
		SetBackgroundColor(Color::SidePanelBackground);

		_pExpandedRoot = CreateControl<Area>();
		_pCollapsedRoot = CreateControl<Area>();

		auto pHeaderPanel = _pExpandedRoot->CreateControl<Panel>();
		pHeaderPanel->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pLogo = pHeaderPanel->CreateControl<Image>(AppResources::GetTexture(Resource::LOGO_SMALL), Color::Black);
		pLogo->SetX(44);

		auto pGradient = _pExpandedRoot->CreateControl<HorizontalGradient>(Color::SidePanelGradient.WithAlpha(0.0f), Color::SidePanelGradient.WithAlpha(0.8f));
		_pGradient = pGradient;

		_pMenuButton = CreateControl<ButtonWithIcon>(Resource::ICON_MENU);
		_pMenuButton->SetTheme(Theme::SidePanelButtonStyle);
		_pMenuButton->SetX(3);
		_pMenuButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pMenuButton->GetHeight()) / 2);
//		_pMenuButton->CenterVertically();
		_pMenuButton->SetDelegate([this]() { ShowMenu(); });

		auto pMainArea = _pExpandedRoot->CreateControl<Area>();

		auto pChatButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_CHATS, toStr(fig::strings::UI::MenuRecentChats));
		pChatButton->SetDelegate([]() { PushEvent(UserEvent::NavigateToChatList); });
		auto pCharactersButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_CHARACTERS, toStr(fig::strings::UI::MenuCharacters));
		pCharactersButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen(ScreenType::Home); });
		auto pScenariosButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_SCENARIOS, toStr(fig::strings::UI::MenuScenarios));
		auto pWorldsButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_WORLDS, toStr(fig::strings::UI::MenuWorlds));
		auto pModelsButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_MODELS, "Models");

		auto pChatButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(Resource::ICON_MENU_CHATS_SMALL, false);
		pChatButtonSmall->SetDelegate([]() { PushEvent(UserEvent::NavigateToChatList); });
		pChatButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pCharactersButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(Resource::ICON_MENU_CHARACTERS_SMALL, false);
		pCharactersButtonSmall->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen(ScreenType::Home); });
		pCharactersButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pScenariosButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(Resource::ICON_MENU_SCENARIOS_SMALL, false);
		pScenariosButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pWorldsButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(Resource::ICON_MENU_WORLDS_SMALL, false);
		pWorldsButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pModelsButtonSmall = _pCollapsedRoot->CreateControl<ButtonWithIcon>(Resource::ICON_MENU_MODELS_SMALL, false);
		pModelsButtonSmall->SetTheme(Theme::SidePanelButtonStyle);

		auto pButtonSizer = pMainArea->SetSizer<VerticalSizer>();
		pButtonSizer->AddSpacer(20);
		pButtonSizer->Add(pChatButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pCharactersButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pScenariosButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pWorldsButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pModelsButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);

		auto pFooterPanel = _pExpandedRoot->CreateControl<Area>();
		pFooterPanel->SetHeight(Constants::GUI::SidePanel::FooterHeight);

		_pModelWidget = pFooterPanel->CreateControl<LoadModelWidget>();
		_pModelWidget->SetHeight(60);
		_pModelWidget->SetMaxHeight(60);

		_pUserWidget = pFooterPanel->CreateControl<UserProfileWidget>();
		_pUserWidget->SetHeight(60);
		
		auto pMainSizer =_pExpandedRoot->SetSizer<VerticalSizer>();
		pMainSizer->Add(pHeaderPanel, 0, SizerFlag::Expand | SizerFlag::Fill);
		pMainSizer->Add(pMainArea, -1, SizerFlag::Fill);
		pMainSizer->Add(_pModelWidget, 0, SizerFlag::Expand);
		pMainSizer->Add(_pUserWidget, 0, SizerFlag::Expand);

		auto pSmallButtonSizer = _pCollapsedRoot->SetSizer<VerticalSizer>();
		pSmallButtonSizer->AddSpacer(56);
		pSmallButtonSizer->Add(pChatButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pCharactersButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pScenariosButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pWorldsButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pSmallButtonSizer->AddSpacer(8);
		pSmallButtonSizer->Add(pModelsButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
//		_pCollapsedRoot->Cull(true);

		_pResizeHandle = CreateControl<ResizeHandle>(Direction::East);
		_pResizeHandle->SetDelegate([this](fig::coord size) { Resize(size); });
		_pResizeHandle->SetClickDelegate([this]() { _bExpanded ? Collapse() : Expand(); });
		_bExpanded = false;
		Expand();
	}

	void SidePanel::OnAfterLayout()
	{
		constexpr fig::coord kGradientSize = 8;
		_pGradient->SetX(GetWidth() - kGradientSize);
		_pGradient->SetSize(kGradientSize, GetHeight());
	}

	void SidePanel::ShowMenu()
	{
		auto& menu = CreateMenu();
		auto& createMenu = menu.AddItem("Create");
		createMenu.AddItem("New character\u2026");
		createMenu.AddItem("New story\u2026");
		createMenu.AddItem("New world\u2026");
		createMenu.AddSeparator();
		createMenu.AddItem("From file\u2026")
			.SetEnabled(false);
		menu.AddSeparator();
		menu.AddItem("User profile\u2026", Resource::ICON_USER_SETTINGS);
		menu.AddItem("Settings\u2026", Resource::ICON_SETTINGS);
		menu.AddSeparator();
		menu.AddItem("Sign out", Resource::ICON_LOGOUT)
			.SetDelegate([]() { MainFrame::GetInstance().SignOut(); });
		menu.AddItem("Exit Figment")
			.SetDelegate([]() { MainFrame::GetInstance().Close(); });

		menu.Show(fig::point { _pMenuButton->GetX(), _pMenuButton->GetY() + _pMenuButton->GetHeight() });
	}

	EventResult SidePanel::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::UserSignedIn))
		{
			_pUserWidget->SetUser(GetUserData<fig::user::UserProfile>(event));

			Global::GetUserSettings().GetBool(UserSetting::Interface::SidePanelCollapsed) ? Collapse() : Expand();
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
		_pExpandedRoot->Cull(false);
		_pCollapsedRoot->Cull(true);

		if (Global::IsSignedIn())
			Global::GetUserSettings().SetBool(UserSetting::Interface::SidePanelCollapsed, false);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pExpandedRoot, -1, SizerFlag::Expand | SizerFlag::Fill);

		PushEvent(UserEvent::SidePanelResized);
	}

	void SidePanel::Collapse() noexcept
	{
		if (not _bExpanded)
			return;
		_bExpanded = false;

		SetWidth(42);
		_pExpandedRoot->Cull(true);
		_pCollapsedRoot->Cull(false);

		if (Global::IsSignedIn())
			Global::GetUserSettings().SetBool(UserSetting::Interface::SidePanelCollapsed, true);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(_pCollapsedRoot, -1, SizerFlag::Expand | SizerFlag::Fill);

		PushEvent(UserEvent::SidePanelResized);
	}

	void SidePanel::OnSize()
	{
		if (_pResizeHandle)
			_pResizeHandle->FillParent();
	}

	void SidePanel::Resize(fig::coord size) noexcept
	{
		if (_bExpanded and size < 120)
			Collapse();
		else if (not _bExpanded and size > 200)
			Expand();
	}
}