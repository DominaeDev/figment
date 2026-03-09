#include <pch.h>
#include "gui/HomeScreen.h"
#include "gui/CardList.h"
#include "gui/MainFrame.h"

namespace fig::gui
{
	HomeScreen::HomeScreen(Frame* pParent) : Screen(pParent)
	{
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		auto pTopBar = new Panel(this);
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto pExpandButton = new ButtonWithIcon(pTopBar, TextureType::ICON_SIDEBAR_EXPAND);
		pExpandButton->SetSize(36, 36);
		pExpandButton->SetX(4.0f);
		pExpandButton->CenterVertically();
		pExpandButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(true); });
		_pExpandButton = pExpandButton;

		_pCardList = new CardList(this);

		auto topSizer = new VerticalSizer();
		topSizer->Add(pTopBar, 0, Sizer::Expand);
		topSizer->Add(_pCardList, -1, Sizer::Expand | Sizer::Left | Sizer::Right, 6);
		SetSizer(topSizer);
	}

	void HomeScreen::OnRender(Renderer* pRenderer)
	{
		DrawBackground(pRenderer);
	}

	bool HomeScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed) // Press
		{
		}
		else // Release
		{
		}
		return false;
	}

	void HomeScreen::CreateCards()
	{
//		auto startTime = std::chrono::steady_clock::now();

		_pCardList->CreateCards(CardList::CardType::Character);

//		auto endTime = std::chrono::steady_clock::now();
//		MainFrame::SetStatusBar(std::format("Duration: {}ms", toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count())));

		InvalidateLayout();
	}

	void HomeScreen::OnSidePanel(bool bShown)
	{
		_pExpandButton->SetVisible(!bShown);
	}
}