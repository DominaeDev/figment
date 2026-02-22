#include <pch.h>
#include "gui/HomeFrame.h"
#include "gui/CardList.h"
#include "gui/MainFrame.h"

namespace fig::gui
{
	HomeFrame::HomeFrame(Frame* pParent) : Screen(pParent)
	{
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		auto pTopBar = new Panel(this);
		pTopBar->SetHeight(48);

		_pCardList = new CardList(this);

		auto topSizer = new VerticalSizer();
		topSizer->Add(pTopBar, 0, Sizer::Expand);
		topSizer->Add(_pCardList, -1, Sizer::Expand | Sizer::Left | Sizer::Right, 6);
		SetSizer(topSizer);
	}

	void HomeFrame::OnRender(Renderer* pRenderer)
	{
		DrawBackground(pRenderer);
	}

	bool HomeFrame::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed) // Press
		{
		}
		else // Release
		{
		}
		return false;
	}

	void HomeFrame::CreateCards()
	{
		auto startTime = std::chrono::steady_clock::now();

		_pCardList->CreateCards(CardList::CardType::Character);

		auto endTime = std::chrono::steady_clock::now();
		double duration = toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

		MainFrame::SetStatusBar(std::format("Duration: {}ms", duration));

		InvalidateLayout();
	}
}