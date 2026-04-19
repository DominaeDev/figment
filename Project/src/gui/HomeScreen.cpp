#include <pch.h>
#include "gui/HomeScreen.h"
#include "gui/CardList.h"
#include "gui/MainFrame.h"
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/Menu.h"
#include "gui/ToggleWithIcon.h"
#include "gui/TexturedBorder.h"
#include "model/AppState.h"
#include "model/UserManager.h"
#include "util/Common.h"

namespace fig::gui
{
	static FilterFlags GetFiltering()
	{ 
		return Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Filtering, DefaultFilterFlags, FilterFlagMapping);
	};

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
//		_pFilterBorder = new TexturedBorder(_pFilteringButton, TextureType::ROUNDED_BACKGROUND_6PX, 8);

		_pGridButton = new ToggleWithIcon(pTopBar, TextureType::ICON_GRID_LARGE);
		_pGridButton->SetDelegate([this](bool _) { ToggleCardSize(); });

		_pToggleTagsButton = new ButtonWithIcon(pTopBar, TextureType::ICON_TAG);
		_pToggleTagsButton->SetDelegate([this]() { ToggleTags(); });

		_pFilterTextBox = new SearchBox(pTopBar, FontFace::Default, 16.0);
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(192, 30);
		_pFilterTextBox->SetMaxSize(192, -1);
		_pFilterTextBox->SetBackgroundColor(Colors::White);
		_pFilterTextBox->SetTextChangedCallback([this](fig::string s) {
			this->OnSearchFilter(s); 
		});

		auto pTopSizer = new HorizontalSizer();
		pTopSizer->Add(pExpandButton, 0, Sizer::AlignCenterVertical | Sizer::Left, 4);
		pTopSizer->Add(pHomeButton, 0, Sizer::AlignCenterVertical | Sizer::Left, 8);
		pTopSizer->Add(_pHeader, 0, Sizer::AlignCenterVertical | Sizer::Left, 6);
		pTopSizer->AddStretchSpacer();
		pTopSizer->Add(_pGridButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(_pToggleTagsButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
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

	void HomeScreen::OnUserSignedIn(const fig::user::UserProfile& profile)
	{
		bool bHalfSize = Global::GetUserSettings().GetBool(UserSetting::HalfSizeCards);
		_pCardList->SetCardSize(bHalfSize ? CardSize::Half : CardSize::Full);
		_pCardList->EnableTags(Global::GetUserSettings().GetBool(UserSetting::ShowTags));
		_pGridButton->SetIcon(bHalfSize ? TextureType::ICON_GRID_SMALL : TextureType::ICON_GRID_LARGE);
		_pGridButton->Toggle(bHalfSize, false);
		_pToggleTagsButton->EnableBorder(Global::GetUserSettings().GetBool(UserSetting::ShowTags));
		_pFilteringButton->EnableBorder(GetFiltering() != DefaultFilterFlags);
	}

	CardList& HomeScreen::GetCardList()
	{
		return *_pCardList;
	}

	void HomeScreen::OnSearchFilter(fig::string search_text)
	{
		if (search_text.size() < 2)
		{
			_pCardList->SetFilter("");
			_search_text.clear();
			_fSearchTimer = 0.0f;
		}
		else
		{
			_search_text = std::move(search_text);
			_fSearchTimer = 0.005f;
		}
	}

	void HomeScreen::ToggleCardSize() noexcept
	{
		bool bSmall = Global::GetUserSettings().GetBool(UserSetting::HalfSizeCards);
		bSmall = !bSmall;
		Global::GetUserSettings().SetBool(UserSetting::HalfSizeCards, bSmall);

		_pGridButton->SetIcon(bSmall ? TextureType::ICON_GRID_SMALL : TextureType::ICON_GRID_LARGE);
		_pCardList->SetCardSize(bSmall ? CardSize::Half : CardSize::Full);
	}

	void HomeScreen::ToggleTags() noexcept
	{
		_pCardList->EnableTags(!_pCardList->IsTagsEnabled());
		Global::GetUserSettings().SetBool(UserSetting::ShowTags, _pCardList->IsTagsEnabled());
		_pToggleTagsButton->EnableBorder(_pCardList->IsTagsEnabled());
	}

	void HomeScreen::ShowSortingMenu() noexcept
	{
		auto ChangeSorting = [this](SortBy sorting) {
			Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Sorting, sorting);
			_pCardList->Reorder();
		};

		auto ChangeOrdering = [this](OrderBy ordering) {
			Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Ordering, ordering);
			_pCardList->Reorder();
		};

		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Ordering, OrderBy::Default);

		auto& menu = MainFrame::GetInstance().CreateMenu();
		menu.AddCheckItem("Sort alphabetically", sortBy == SortBy::Name)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::Name); });
		menu.AddCheckItem("Sort by recency", sortBy == SortBy::LastUsedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::LastUsedAt); });
		menu.AddCheckItem("Sort by creation date", sortBy == SortBy::CreatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::CreatedAt); });
		menu.AddCheckItem("Sort by update date", sortBy == SortBy::UpdatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::UpdatedAt); });
		menu.AddCheckItem("Sort by chats", sortBy == SortBy::ChatCount)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::ChatCount); });
		menu.AddSeparator();
		menu.AddCheckItem("Ascending", orderBy == OrderBy::Ascending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Ascending); });
		menu.AddCheckItem("Descending", orderBy == OrderBy::Descending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Descending); });
		menu.AddSeparator();
		menu.AddItem("Reset")
			.SetDelegate([this] { 
				Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Sorting, SortBy::Default);
				Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Ordering, OrderBy::Default);
				_pCardList->Reorder();
			});

		menu.Show(Point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void HomeScreen::ShowFilteringMenu() noexcept
	{
		auto SetFilter = [this](FilterFlags filtering) {
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Filtering, filtering, FilterFlagMapping);
			_pCardList->Reorder();
			_pFilteringButton->EnableBorder(GetFiltering() != DefaultFilterFlags);
		};

		auto ToggleFilter = [this](FilterFlag flag) {
			auto filtering = GetFiltering();
			filtering.Flip(flag);
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Filtering, filtering, FilterFlagMapping);
			_pCardList->Reorder();
			_pFilteringButton->EnableBorder(GetFiltering() != DefaultFilterFlags);
		};

		auto filtering = GetFiltering();
		bool bShowHidden = filtering.IsSet(FilterFlag::Hidden);

		auto& menu = MainFrame::GetInstance().CreateMenu();
		menu.AddCheckItem("New", filtering.IsSet(FilterFlag::New))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::New); });
		menu.AddCheckItem("Starred", filtering.IsSet(FilterFlag::Starred))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::Starred); });
		menu.AddCheckItem("At least one chat", filtering.IsSet(FilterFlag::Chats))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::Chats); });
		menu.AddSeparator();
		auto& genders = menu.AddItem("By gender");
		genders.AddCheckItem("Show male", filtering.IsSet(FilterFlag::GenderMale))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::GenderMale); });
		genders.AddCheckItem("Show female", filtering.IsSet(FilterFlag::GenderFemale))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::GenderFemale); });
		genders.AddCheckItem("Show non-binary", filtering.IsSet(FilterFlag::GenderOther))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::GenderOther); });
		auto& sources = menu.AddItem("By source");
		sources.AddCheckItem("Show created", filtering.IsSet(FilterFlag::SourceCreated))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::SourceCreated); });
		sources.AddCheckItem("Show imported", filtering.IsSet(FilterFlag::SourceImported))
			.SetEnabled(!bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::SourceImported); });
		menu.AddSeparator();
		menu.AddCheckItem("Show hidden", bShowHidden)
			.SetDelegate([ToggleFilter, this] { ToggleFilter(FilterFlag::Hidden); });
		menu.AddSeparator();
		menu.AddItem("Reset")
			.SetDelegate([SetFilter, this] { SetFilter(DefaultFilterFlags); });
		menu.Show(Point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}
	
}