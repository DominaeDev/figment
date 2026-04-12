#include <pch.h>
#include "gui/HomeScreen.h"
#include "gui/CardList.h"
#include "gui/MainFrame.h"
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/Menu.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "util/Common.h"

namespace fig::gui
{
	HomeScreen::HomeScreen(Frame* pParent) : Screen(pParent)
	{
		auto pTopBar = new Panel(this);
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		_pHeader = new StaticText(pTopBar, "Characters", FontFace::Italic, 24, false);
		_pHeader->SetX(52);
		_pHeader->SetHeight(Constants::GUI::SidePanel::HeaderHeight);
		_pHeader->SetAlignment(TextAlignment::Left_Center);
		
		auto pExpandButton = new ButtonWithIcon(pTopBar, TextureType::ICON_SIDEBAR);
		pExpandButton->SetDelegate([]() { MainFrame::GetInstance().ShowSidePanel(true); });
		_pExpandButton = pExpandButton;

		auto pHomeButton = new ButtonWithIcon(pTopBar, TextureType::ICON_HOME);

		_pSortingButton = new ButtonWithIcon(pTopBar, TextureType::ICON_SORTING);
		_pSortingButton->SetDelegate([this]() { ShowSortingMenu(); });
		_pFilteringButton = new ButtonWithIcon(pTopBar, TextureType::ICON_FILTERING);
		_pFilteringButton->SetDelegate([this]() { ShowFilteringMenu(); });
		auto toggleTagsButton = new ButtonWithIcon(pTopBar, TextureType::ICON_TAG);
		toggleTagsButton->SetDelegate([this]() { ToggleTags(); });
		auto gridLargeButton = new ButtonWithIcon(pTopBar, TextureType::ICON_GRID_LARGE);
		gridLargeButton->SetDelegate([this]() { SetSmallCardSize(false); });
		auto gridSmallButton = new ButtonWithIcon(pTopBar, TextureType::ICON_GRID_SMALL);
		gridSmallButton->SetDelegate([this]() { SetSmallCardSize(true); });

		_pFilterTextBox = new SearchBox(pTopBar, FontFace::Default, 16.0);
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(220, 30);
		_pFilterTextBox->SetMaxSize(220, -1);
		_pFilterTextBox->SetBackgroundColor(Colors::White);
		_pFilterTextBox->SetTextChangedCallback([this](fig::string s) {
			this->OnFilter(s); 
		});

		auto pTopSizer = new HorizontalSizer();
		pTopSizer->Add(pExpandButton, 0, Sizer::AlignCenterVertical | Sizer::Left, 4);
		pTopSizer->Add(pHomeButton, 0, Sizer::AlignCenterVertical | Sizer::Left, 8);
		pTopSizer->Add(_pHeader, 0, Sizer::AlignCenterVertical | Sizer::Left, 6);
		pTopSizer->AddStretchSpacer();
		pTopSizer->Add(gridLargeButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(gridSmallButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(toggleTagsButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(_pSortingButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(_pFilteringButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 8);
		pTopSizer->Add(_pFilterTextBox, 0, Sizer::AlignCenterVertical | Sizer::Right, 8);
		pTopBar->SetSizer(pTopSizer);

		_pCardList = new CardList(this);
		_pCardList->SetScrollBarOffset(0);

		auto mainSizer = new VerticalSizer();
		mainSizer->Add(pTopBar, 0, Sizer::Expand);
		mainSizer->Add(_pCardList, -1, Sizer::Fill | Sizer::Left | Sizer::Right, 16);
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
		if (event.pressed)
		{
		}
		else // Released
		{
		}
		return false;
	}

	void HomeScreen::CreateCards()
	{
		_pCardList->SetCardSize(Global::GetUserSettings().GetBool(UserSetting::HalfSizeCards) ? CardSize::Half : CardSize::Full);
		_pCardList->EnableTags(Global::GetUserSettings().GetBool(UserSetting::ShowTags));

		DEBUG_MEASURE_BEGIN("CreateCards");
		_pCardList->CreateCards(CardList::CardType::Character);
		DEBUG_MEASURE_END();

		InvalidateLayout();
	}

	void HomeScreen::OnSidePanel(bool bShown)
	{
		_pExpandButton->SetVisible(!bShown);
		_pExpandButton->EnableLayout(!bShown);
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

	void HomeScreen::SetSmallCardSize(bool bSmall) noexcept
	{
		_pCardList->SetCardSize(bSmall ? CardSize::Half : CardSize::Full);
		Global::GetUserSettings().SetBool(UserSetting::HalfSizeCards, bSmall);
	}

	void HomeScreen::ToggleTags()
	{
		_pCardList->EnableTags(!_pCardList->IsTagsEnabled());
		Global::GetUserSettings().SetBool(UserSetting::ShowTags, _pCardList->IsTagsEnabled());
	}

	void HomeScreen::ShowSortingMenu()
	{
		MainFrame::GetInstance().DestroyOverlays(); //! @menu

		auto ChangeSorting = [](CardList* pCardList, SortBy sorting) {
			Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Sorting, sorting);
			pCardList->Reorder();
		};

		auto ChangeOrdering = [](CardList* pCardList, OrderBy ordering) {
			Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Ordering, ordering);
			pCardList->Reorder();
		};

		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Sorting, SortBy::CreatedAt);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Ordering, OrderBy::Descending);

		auto pMenu = new Menu(&MainFrame::GetInstance());
		pMenu->AddCheckItem("Sort by name", sortBy == SortBy::Name)
			.SetDelegate([&]() { ChangeSorting(_pCardList, SortBy::Name); });
		pMenu->AddCheckItem("Sort by creation date", sortBy == SortBy::CreatedAt)
			.SetDelegate([&]() { ChangeSorting(_pCardList, SortBy::CreatedAt); });
		pMenu->AddCheckItem("Sort by last updated", sortBy == SortBy::UpdatedAt)
			.SetDelegate([&]() { ChangeSorting(_pCardList, SortBy::UpdatedAt); });
		pMenu->AddCheckItem("Sort by most recent chat", sortBy == SortBy::LastMessaged)
			.SetDelegate([&]() { ChangeSorting(_pCardList, SortBy::LastMessaged); });
		pMenu->AddCheckItem("Sort by chat count", sortBy == SortBy::ChatCount)
			.SetDelegate([&]() { ChangeSorting(_pCardList, SortBy::ChatCount); });
		pMenu->AddSeparator();
		pMenu->AddCheckItem("Ascending", orderBy == OrderBy::Ascending)
			.SetDelegate([&]() { ChangeOrdering(_pCardList, OrderBy::Ascending); });
		pMenu->AddCheckItem("Descending", orderBy == OrderBy::Descending)
			.SetDelegate([&]() { ChangeOrdering(_pCardList, OrderBy::Descending); });

		pMenu->Show(Point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void HomeScreen::ShowFilteringMenu()
	{
		MainFrame::GetInstance().DestroyOverlays(); //! @menu

		auto pMenu = new Menu(&MainFrame::GetInstance());
		pMenu->AddCheckItem("Filter by new");
		pMenu->AddCheckItem("Filter by starred");
		pMenu->AddCheckItem("Filter by chats");
		pMenu->AddSeparator();
		pMenu->AddCheckItem("Show male", true);
		pMenu->AddCheckItem("Show female", true);
		pMenu->AddCheckItem("Show non-binary", true);
		pMenu->AddSeparator();
		pMenu->AddCheckItem("Show hidden");

		pMenu->Show(Point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}
	
}