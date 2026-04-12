#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"
#include "gui/SidePanelButton.h"
#include "gui/UserProfileWidget.h"
#include "gui/HomeScreen.h"
#include "gui/ChatScreen.h"
#include "gui/LineBorderRenderer.h"
#include "gui/Menu.h"
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

		auto pGradient = new HorizontalGradient(this, Colors::SidePanelGradient.WithAlpha(0.0f), Colors::SidePanelGradient.WithAlpha(0.8f));
		_pGradient = pGradient;

		_pMenuButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_MENU);
		_pMenuButton->SetTheme(Themes::SidePanelButtonStyle);
		_pMenuButton->SetSize(36, 36);
		_pMenuButton->SetX(4);
		_pMenuButton->CenterVertically();
		_pMenuButton->SetDelegate([this]() { ShowMenu(); });

/*		auto pSettingsButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_SETTINGS);
		pSettingsButton->SetTheme(Themes::SidePanelButtonStyle);
		pSettingsButton->SetSize(36, 36);
		pSettingsButton->SetX(GetWidth() - pSettingsButton->GetWidth() - 40);
		pSettingsButton->CenterVertically(); 
*/

		auto pCollapseButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_SIDEBAR);
		pCollapseButton->SetTheme(Themes::SidePanelButtonStyle);
		pCollapseButton->SetSize(36, 36);
		pCollapseButton->SetX(GetWidth() - pCollapseButton->GetWidth() - 4);
		pCollapseButton->CenterVertically();
		pCollapseButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(false); });

		auto pMainArea = new Area(this);

		auto pChatButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_CHATS, toStr(fig::strings::UI::MenuRecentChats));
		pChatButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen<ChatScreen>(); });
		auto pCharactersButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_CHARACTERS, toStr(fig::strings::UI::MenuCharacters));
		pCharactersButton->SetDelegate([]() { MainFrame::GetInstance().ChangeScreen<HomeScreen>(); });
		auto pScenariosButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_SCENARIOS, toStr(fig::strings::UI::MenuScenarios));
		auto pModelsButton = new SidePanelButton(pMainArea, TextureType::ICON_MENU_MODELS, "Models");

		auto pButtonSizer = new VerticalSizer();
		pMainArea->SetSizer(pButtonSizer);

		pButtonSizer->AddSpacer(20);
		pButtonSizer->Add(pChatButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pCharactersButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pScenariosButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);
		pButtonSizer->AddSpacer(4);
		pButtonSizer->Add(pModelsButton, 0, Sizer::Expand | Sizer::Right | Sizer::Left, 12);

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

		SetBorderRenderer(new LineBorderRenderer(Colors::LineColor, { Direction::East }));
	}

	void SidePanel::OnAfterLayout()
	{
		constexpr Coord kGradientSize = 8;
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

	void SidePanel::ShowMenu()
	{
		MainFrame::GetInstance().DestroyOverlays(); //! @menu

		auto pMenu = new Menu(&MainFrame::GetInstance());
		pMenu->AddItem("New character\u2026");
		pMenu->AddItem("New scenario\u2026");
		pMenu->AddItem("Import character\u2026")
			.SetEnabled(false);
		pMenu->AddSeparator();
		pMenu->AddItem("Profile settings\u2026", TextureType::ICON_USER_SETTINGS);
		pMenu->AddItem("Preferences\u2026", TextureType::ICON_SETTINGS);
		pMenu->AddSeparator();
		pMenu->AddItem("Sign out", TextureType::ICON_LOGOUT)
			.SetDelegate([]() { MainFrame::GetInstance().SignOut(); });
		pMenu->AddItem("Exit Figment")
			.SetDelegate([]() { MainFrame::GetInstance().Close(); });

		pMenu->Show(Point { _pMenuButton->GetX() + 6, _pMenuButton->GetY() + _pMenuButton->GetHeight() + 2 });
	}

}