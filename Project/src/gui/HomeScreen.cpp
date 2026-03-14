#include <pch.h>
#include "gui/HomeScreen.h"
#include "gui/CardList.h"
#include "gui/MainFrame.h"
#include "gui/SearchBox.h"

namespace fig::gui
{
	HomeScreen::HomeScreen(Frame* pParent) : Screen(pParent)
	{
		SetForegroundColor(Colors::Black);
		SetBackgroundColor(Colors::AppBackground);

		auto pTopBar = new Panel(this);
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		_pFilterTextBox = new SearchBox(pTopBar, FontFace::Default, 16.0);
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(250, 30);
		_pFilterTextBox->SetMaxSize(250, -1);
		_pFilterTextBox->SetBackgroundColor(Colors::White);
		_pFilterTextBox->SetTextChangedCallback([this](fig::string s) {
			this->OnFilter(s); 
		});

		auto pTopSizer = new HorizontalSizer();
		pTopSizer->AddStretchSpacer();
		pTopSizer->Add(_pFilterTextBox, 0, Sizer::AlignCenterVertical | Sizer::Right, 8);
		pTopBar->SetSizer(pTopSizer);

		auto pExpandButton = new ButtonWithIcon(pTopBar, TextureType::ICON_SIDEBAR_EXPAND);
		pExpandButton->SetSize(36, 36);
		pExpandButton->SetX(4.0f);
		pExpandButton->CenterVertically();
		pExpandButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(true); });
		_pExpandButton = pExpandButton;

		_pCardList = new CardList(this);

		auto mainSizer = new VerticalSizer();
		mainSizer->Add(pTopBar, 0, Sizer::Expand);
		mainSizer->Add(_pCardList, -1, Sizer::Expand | Sizer::Left | Sizer::Right, 6);
		SetSizer(mainSizer);
	}

	void HomeScreen::OnUpdate(float fElapsed)
	{
		if (_fSearchTimer > 0.0f)
		{
			if ((_fSearchTimer -= fElapsed) <= 0.0f)
				_pCardList->SetFilter(_search_text);
		}
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
		auto startTime = std::chrono::steady_clock::now();

		_pCardList->CreateCards(CardList::CardType::Character);

		auto endTime = std::chrono::steady_clock::now();
		MainFrame::SetStatusBar(std::format("Duration: {}ms", toD(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count())));

		InvalidateLayout();
	}

	void HomeScreen::OnSidePanel(bool bShown)
	{
		_pExpandButton->SetVisible(!bShown);
	}

	CardList& HomeScreen::GetCardList()
	{
		return *_pCardList;
	}

	void HomeScreen::OnFilter(fig::string search_text)
	{
		if (search_text.size() < 2)
		{
			_pCardList->ClearFilter();
			_fSearchTimer = 0.0f;
		}
		else
		{
			_search_text = std::move(search_text);
			_fSearchTimer = 0.2f;
		}
	}


}