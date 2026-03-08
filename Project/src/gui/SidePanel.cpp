#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	SidePanel::SidePanel(LayoutElement* pParent) : Control(pParent)
	{
		SetWidth(Constants::GUI::SidePanel::Width);
		SetBackgroundColor(Color { 0xEE, 0xEC, 0xE4, 0xFF });

		auto pHeaderPanel = new Panel(this);
		pHeaderPanel->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pLogo = new Image(pHeaderPanel, AppResources::GetTexture(TextureType::LOGO_SMALL), Colors::Black);
		pLogo->SetX(44);

		auto pMenuButton = new Panel(pHeaderPanel);
		pMenuButton->SetSize(36, 36);
		pMenuButton->SetX(4.0f);
		pMenuButton->CenterVertically();
		auto pMenuIcon = new Image(pMenuButton, AppResources::GetTexture(TextureType::ICON_MENU), Colors::Black);
		pMenuIcon->Center();

		auto pCollapseButton = new Panel(pHeaderPanel);
		pCollapseButton->SetSize(36, 36);
		pCollapseButton->SetX(GetWidth() - pCollapseButton->GetWidth() - 4.0f);
		pCollapseButton->CenterVertically();
		auto pCollapseIcon = new Image(pCollapseButton, AppResources::GetTexture(TextureType::ICON_SIDEBAR), Colors::Black);
		pCollapseIcon->Center();

		auto pMainArea = new Area(this);

		auto pFooterPanel = new Panel(this);
		pFooterPanel->SetHeight(Constants::GUI::SidePanel::FooterHeight);
		
		auto pTopSizer = new VerticalSizer();
		pTopSizer->Add(pHeaderPanel, 0, Sizer::Expand);
		pTopSizer->Add(pMainArea, -1, Sizer::Expand);
		pTopSizer->Add(pFooterPanel, 0, Sizer::Expand);
		
		SetSizer(pTopSizer);
	}
}