#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"
#include "gui/SidePanelButton.h"

#include "gui/HomeScreen.h"
#include "gui/ChatScreen.h"
#include "model/GlobalStrings.h"

using namespace fig::gui::util;

namespace fig::gui
{
	SidePanel::SidePanel(LayoutElement* pParent) : Control(pParent)
	{
		SetWidth(Constants::GUI::SidePanel::Width);
		SetBackgroundColor(Colors::SidePanelBackground);

		auto pHeaderPanel = new Panel(this);
		pHeaderPanel->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pLogo = new Image(pHeaderPanel, AppResources::GetTexture(TextureType::LOGO_SMALL), Colors::Black);
		pLogo->SetX(44);

		auto pGradient = new HorizontalGradient(this, with_alpha(Colors::SidePanelGradient, 0.0f), with_alpha(Colors::SidePanelGradient, 0.6f));
		_pGradient = pGradient;

		auto pMenuButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_MENU);
		pMenuButton->SetSize(36, 36);
		pMenuButton->SetX(4.0f);
		pMenuButton->CenterVertically();

		auto pSettingsButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_SETTINGS);
		pSettingsButton->SetSize(36, 36);
		pSettingsButton->SetX(GetWidth() - pSettingsButton->GetWidth() - 40.0f);
		pSettingsButton->CenterVertically();

		auto pCollapseButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_SIDEBAR_COLLAPSE);
		pCollapseButton->SetSize(36, 36);
		pCollapseButton->SetX(GetWidth() - pCollapseButton->GetWidth() - 4.0f);
		pCollapseButton->CenterVertically();
		pCollapseButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(false); });

		auto pMainArea = new Area(this);

		auto pChatButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_CHATS, toStr(fig::strings::UI::MenuRecentChats));
		pChatButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen<ChatScreen>(); });
		pChatButton->SetHeight(58);

		auto pCharactersButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_CHARACTERS, toStr(fig::strings::UI::MenuCharacters));
		pCharactersButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen<HomeScreen>(); });
		pCharactersButton->SetHeight(58);

		auto pScenariosButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_SCENARIOS, toStr(fig::strings::UI::MenuScenarios));
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
		
		auto pTopSizer = new VerticalSizer();
		pTopSizer->Add(pHeaderPanel, 0, Sizer::Expand);
		pTopSizer->Add(pMainArea, -1, Sizer::Expand);
		pTopSizer->Add(pFooterPanel, 0, Sizer::Expand);
		
		SetSizer(pTopSizer);
	}

	void SidePanel::OnAfterLayout()
	{
		const float kGradientSize = 7.0f;
		_pGradient->SetX(GetWidth() - kGradientSize);
		_pGradient->SetSize(kGradientSize, GetHeight());
	}
}