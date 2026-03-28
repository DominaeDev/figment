#include <pch.h>
#include "gui/HomeScreen.h"
#include "gui/CardList.h"
#include "gui/MainFrame.h"
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "util/Common.h"

namespace fig::gui
{
	HomeScreen::HomeScreen(Frame* pParent) : Screen(pParent)
	{
		auto pTopBar = new Panel(this);
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);
		
		auto toggleTagsButton = new ButtonWithIcon(pTopBar, TextureType::ICON_TAG);
		toggleTagsButton->SetDelegate([this]() { ToggleTags(); });
		auto gridLargeButton = new ButtonWithIcon(pTopBar, TextureType::ICON_GRID_LARGE);
		gridLargeButton->SetDelegate([this]() { SetSmallGridSize(false); });
		auto gridSmallButton = new ButtonWithIcon(pTopBar, TextureType::ICON_GRID_SMALL);
		gridSmallButton->SetDelegate([this]() { SetSmallGridSize(true); });

		_pFilterTextBox = new SearchBox(pTopBar, FontFace::Default, 16.0);
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(220, 30);
		_pFilterTextBox->SetMaxSize(220, -1);
		_pFilterTextBox->SetBackgroundColor(Colors::White);
		_pFilterTextBox->SetTextChangedCallback([this](fig::string s) {
			this->OnFilter(s); 
		});

		auto pTopSizer = new HorizontalSizer();
		pTopSizer->AddStretchSpacer();
		pTopSizer->Add(toggleTagsButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(gridLargeButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(gridSmallButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 8);
		pTopSizer->Add(_pFilterTextBox, 0, Sizer::AlignCenterVertical | Sizer::Right, 8);
		pTopBar->SetSizer(pTopSizer);

		auto pExpandButton = new ButtonWithIcon(pTopBar, TextureType::ICON_SIDEBAR);
		pExpandButton->SetSize(36, 36);
		pExpandButton->SetX(4);
		pExpandButton->CenterVertically();
		pExpandButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(true); });
		_pExpandButton = pExpandButton;

		_pCardList = new CardList(this);
//		_pCardList->SetCardSize(CardSize::Half);

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
		DEBUG_MEASURE_BEGIN("CreateCards");
		_pCardList->CreateCards(CardList::CardType::Character);
		DEBUG_MEASURE_END();

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
			_fSearchTimer = 0.005f;
		}
	}

	void HomeScreen::SetSmallGridSize(bool bSmall)
	{
		_pCardList->SetCardSize(bSmall ? CardSize::Half : CardSize::Default);
	}

	void HomeScreen::ToggleTags()
	{
		_pCardList->EnableTags(!_pCardList->IsTagsEnabled());
	}
}