#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"
#include "gui/SidePanelButton.h"
#include "gui/UserProfileWidget.h"

#include "gui/HomeScreen.h"
#include "gui/ChatScreen.h"
#include "model/GlobalStrings.h"

namespace fig::gui
{
	SidePanel::SidePanel(LayoutElement* pParent) : Control(pParent)
	{
		SetWidth(Constants::GUI::SidePanel::Width);
		SetBackgroundColor(fig::gui::Colors::SidePanelBackground);

		auto pHeaderPanel = new Panel(this);
		pHeaderPanel->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pLogo = new Image(pHeaderPanel, AppResources::GetTexture(TextureType::LOGO_SMALL), Colors::Black);
		pLogo->SetX(44);

		auto pGradient = new HorizontalGradient(this, Colors::SidePanelGradient.WithAlpha(0.0f), Colors::SidePanelGradient.WithAlpha(0.6f));
		_pGradient = pGradient;

		auto pMenuButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_MENU);
		pMenuButton->SetTheme(Themes::SidePanelButtonStyle);
		pMenuButton->SetSize(36, 36);
		pMenuButton->SetX(4);
		pMenuButton->CenterVertically();

		auto pSettingsButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_SETTINGS);
		pSettingsButton->SetTheme(Themes::SidePanelButtonStyle);
		pSettingsButton->SetSize(36, 36);
		pSettingsButton->SetX(GetWidth() - pSettingsButton->GetWidth() - 40);
		pSettingsButton->CenterVertically();

		auto pCollapseButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_SIDEBAR);
		pCollapseButton->SetTheme(Themes::SidePanelButtonStyle);
		pCollapseButton->SetSize(36, 36);
		pCollapseButton->SetX(GetWidth() - pCollapseButton->GetWidth() - 4);
		pCollapseButton->CenterVertically();
		pCollapseButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(false); });

		auto pMainArea = new Area(this);

		auto pChatButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_CHATS, toStr(fig::strings::UI::MenuRecentChats));
		pChatButton->SetTheme(Themes::SidePanelButtonStyle);
		pChatButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen<ChatScreen>(); });
		pChatButton->SetHeight(58);

		auto pCharactersButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_CHARACTERS, toStr(fig::strings::UI::MenuCharacters));
		pCharactersButton->SetTheme(Themes::SidePanelButtonStyle);
		pCharactersButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen<HomeScreen>(); });
		pCharactersButton->SetHeight(58);

		auto pScenariosButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_SCENARIOS, toStr(fig::strings::UI::MenuScenarios));
		pScenariosButton->SetTheme(Themes::SidePanelButtonStyle);
		pScenariosButton->SetHeight(58);

		auto pButtonSizer = new VerticalSizer();
		pMainArea->SetSizer(pButtonSizer);

		pButtonSizer->AddSpacer(20);
		pButtonSizer->Add(pChatButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pCharactersButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pScenariosButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);

		auto pFooterPanel = new Area(this);
		pFooterPanel->SetHeight(Constants::GUI::SidePanel::FooterHeight);

		_pUserWidget = new UserProfileWidget(pFooterPanel);
		_pUserWidget->SetHeight(60);

		auto pFooterSizer = new VerticalSizer();
		pFooterSizer->AddStretchSpacer();
		pFooterSizer->Add(_pUserWidget, 0, Sizer::Expand);
		pFooterPanel->SetSizer(pFooterSizer);
		
		auto pTopSizer = new VerticalSizer();
		pTopSizer->Add(pHeaderPanel, 0, Sizer::Expand);
		pTopSizer->Add(pMainArea, -1, Sizer::Fill);
		pTopSizer->Add(pFooterPanel, 0, Sizer::Expand);
		
		SetSizer(pTopSizer);
	}

	void SidePanel::OnAfterLayout()
	{
		constexpr Coord kGradientSize = 7;
		_pGradient->SetX(GetWidth() - kGradientSize);
		_pGradient->SetSize(kGradientSize, GetHeight());
	}

	void SidePanel::SetUserProfile(const fig::user::UserProfile& profile)
	{
		_pUserWidget->SetUser(profile);
	}

	void SidePanel::Reset()
	{
		_pUserWidget->Reset();
	}

}