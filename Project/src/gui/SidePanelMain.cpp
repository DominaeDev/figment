#include <pch.h>
#include "gui/SidePanelMain.h"
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
	SidePanelMain::SidePanelMain(ControlPtr pParent) : SidePanelContent(pParent)
	{
	}

	void SidePanelMain::ShowExpanded()
	{
		DestroyChildren();

		auto pHeaderPanel = CreateControl<Panel>();
		pHeaderPanel->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pLogo = pHeaderPanel->CreateControl<Image>(AppResources::GetTexture(Resource::LOGO_SMALL), Color::Black);
		pLogo->SetX(44);

		_pMenuButton = CreateControl<ButtonWithIcon>(Resource::ICON_MENU);
		_pMenuButton->SetTheme(Theme::SidePanelButtonStyle);
		_pMenuButton->SetX(3);
		_pMenuButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pMenuButton->GetHeight()) / 2);
//		_pMenuButton->CenterVertically();
		_pMenuButton->SetDelegate([this]() { ShowMenu(); });

		auto pMainArea = CreateControl<Area>();

		auto pChatButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_CHATS, toStr(fig::strings::UI::MenuRecentChats));
		pChatButton->SetDelegate([]() { PushEvent(UserEvent::NavigateToChatList); });
		auto pCharactersButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_CHARACTERS, toStr(fig::strings::UI::MenuCharacters));
		pCharactersButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen(ScreenType::Home); });
		auto pScenariosButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_SCENARIOS, toStr(fig::strings::UI::MenuScenarios));
		auto pWorldsButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_WORLDS, toStr(fig::strings::UI::MenuWorlds));
		auto pModelsButton = pMainArea->CreateControl<SidePanelButton>(Resource::ICON_MENU_MODELS, "Models");

		auto pButtonSizer = pMainArea->SetSizer<VerticalSizer>();
		pButtonSizer->Add(pChatButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pCharactersButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pScenariosButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pWorldsButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pModelsButton, 0, SizerFlag::Expand | SizerFlag::Right | SizerFlag::Left, 12);

		auto pFooterPanel = CreateControl<Area>();
		pFooterPanel->SetHeight(Constants::GUI::SidePanel::FooterHeight);

		_pModelWidget = pFooterPanel->CreateControl<LoadModelWidget>();
		_pModelWidget->SetHeight(60);
		_pModelWidget->SetMaxHeight(60);

		_pUserWidget = pFooterPanel->CreateControl<UserProfileWidget>();
		_pUserWidget->SetHeight(60);
		if (auto user = Global::GetUserManager().GetActiveProfile())
			_pUserWidget->SetUser(user.value());

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->Add(pHeaderPanel, 0, SizerFlag::Expand | SizerFlag::Fill);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pMainArea, -1, SizerFlag::Fill);
		pTopSizer->Add(_pModelWidget, 0, SizerFlag::Expand);
		pTopSizer->Add(_pUserWidget, 0, SizerFlag::Expand);
	}

	void SidePanelMain::ShowCollapsed()
	{
		DestroyChildren();

		_pMenuButton = CreateControl<ButtonWithIcon>(Resource::ICON_MENU);
		_pMenuButton->SetTheme(Theme::SidePanelButtonStyle);
		_pMenuButton->SetX(3);
		_pMenuButton->SetY((Constants::GUI::SidePanel::HeaderHeight - _pMenuButton->GetHeight()) / 2);
//		_pMenuButton->CenterVertically();
		_pMenuButton->SetDelegate([this]() { ShowMenu(); });

		auto pChatButtonSmall = CreateControl<ButtonWithIcon>(Resource::ICON_MENU_CHATS_SMALL, false);
		pChatButtonSmall->SetDelegate([]() { PushEvent(UserEvent::NavigateToChatList); });
		pChatButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pCharactersButtonSmall = CreateControl<ButtonWithIcon>(Resource::ICON_MENU_CHARACTERS_SMALL, false);
		pCharactersButtonSmall->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen(ScreenType::Home); });
		pCharactersButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pScenariosButtonSmall = CreateControl<ButtonWithIcon>(Resource::ICON_MENU_SCENARIOS_SMALL, false);
		pScenariosButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pWorldsButtonSmall = CreateControl<ButtonWithIcon>(Resource::ICON_MENU_WORLDS_SMALL, false);
		pWorldsButtonSmall->SetTheme(Theme::SidePanelButtonStyle);
		auto pModelsButtonSmall = CreateControl<ButtonWithIcon>(Resource::ICON_MENU_MODELS_SMALL, false);
		pModelsButtonSmall->SetTheme(Theme::SidePanelButtonStyle);

		auto pTopSizer = SetSizer<VerticalSizer>();
		pTopSizer->AddSpacer(62);
		pTopSizer->Add(pChatButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pCharactersButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pScenariosButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pWorldsButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
		pTopSizer->AddSpacer(8);
		pTopSizer->Add(pModelsButtonSmall, 0, SizerFlag::AlignCenterHorizontal);
	}

	void SidePanelMain::ShowMenu()
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

	EventResult SidePanelMain::OnEvent(fig::event& event)
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

		return EventResult::Pass;
	}
}