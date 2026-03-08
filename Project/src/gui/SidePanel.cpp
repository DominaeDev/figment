#include <pch.h>
#include "gui/SidePanel.h"
#include "gui/AppResources.h"
#include "gui/MainFrame.h"

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

		auto pGradient = new HorizontalGradient(this, with_alpha(Colors::SidePanelGradient, 0x00), with_alpha(Colors::SidePanelGradient, 0xA0));
		_pGradient = pGradient;

		auto pMenuButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_MENU);
		pMenuButton->SetSize(36, 36);
		pMenuButton->SetX(4.0f);
		pMenuButton->CenterVertically();

		auto pCollapseButton = new ButtonWithIcon(pHeaderPanel, TextureType::ICON_SIDEBAR);
		pCollapseButton->SetSize(36, 36);
		pCollapseButton->SetX(GetWidth() - pCollapseButton->GetWidth() - 4.0f);
		pCollapseButton->CenterVertically();
		pCollapseButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(false); });

		auto pMainArea = new Area(this);

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