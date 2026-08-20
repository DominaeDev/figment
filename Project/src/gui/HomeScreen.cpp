#include <pch.h>
#include "gui/HomeScreen.h"
#include "gui/CardList.h"
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/Menu.h"
#include "gui/ToggleWithIcon.h"
#include "gui/TexturedBorder.h"
#include "gui/TopBar.h"
#include "app/AppState.h"
#include "user/UserManager.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	static FilterFlags GetFiltering()
	{ 
		return Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, DefaultFilterFlags, FilterFlagMapping);
	};

	HomeScreen::HomeScreen(Frame* pParent) : Screen(pParent)
	{
		_pCardList = CreateControl<CardList>();
		_pCardList->SetScrollBarOffset(0);

		auto pTopBar = CreateControl<TopBar>("Characters", _pCardList);

		_pSortingButton = pTopBar->CreateControl<ButtonWithIcon>(Resource::ICON_SORTING, false);
		_pSortingButton->SetDelegate([this]() { ShowSortingMenu(); });

		_pFilteringButton = pTopBar->CreateControl<ButtonWithIcon>(Resource::ICON_FILTERING, true);
		_pFilteringButton->SetDelegate([this]() { ShowFilteringMenu(); });
		_pFilteringButton->ShowBorder(false);

		_pGridButton = pTopBar->CreateControl<ToggleWithIcon>(Resource::ICON_GRID_LARGE);
		_pGridButton->SetDelegate([this](bool _) { ToggleCardSize(); });

		_pToggleTagsButton = pTopBar->CreateControl<ButtonWithIcon>(Resource::ICON_TAG, true);
		_pToggleTagsButton->SetDelegate([this]() { ToggleTags(); });
		_pToggleTagsButton->ShowBorder(false);

		_pFilterTextBox = pTopBar->CreateControl<SearchBox>();
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(192, 30);
		_pFilterTextBox->SetMaxWidth(192);
		_pFilterTextBox->SetBackgroundColor(Color::White);
		_pFilterTextBox->SetTextChangedCallback([this](fig::string s) {
			OnSearchFilter(s);
		});

		auto pTopBarSizer = pTopBar->GetSizer();
		pTopBarSizer->AddStretchSpacer();
		pTopBarSizer->Add(_pGridButton, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 2);
		pTopBarSizer->Add(_pToggleTagsButton, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 2);
		pTopBarSizer->Add(_pSortingButton, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 2);
		pTopBarSizer->Add(_pFilteringButton, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 8);
		pTopBarSizer->Add(_pFilterTextBox, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 8);

		auto mainSizer = SetSizer<VerticalSizer>();
		mainSizer->Add(pTopBar, 0, SizerFlag::Expand);
		mainSizer->Add(_pCardList, -1, SizerFlag::Fill | SizerFlag::Left | SizerFlag::Right, 16);
	}

	void HomeScreen::OnUpdate(float fElapsed)
	{
		if (_fSearchTimer > 0.0f)
		{
			if ((_fSearchTimer -= fElapsed) <= 0.0f)
				_pCardList->SetFilter(_search_text);
		}
	}

	void HomeScreen::OnRender(fig::renderer_ptr pRenderer)
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

	void HomeScreen::OnUserSignedIn(const fig::user::UserProfile& profile)
	{
		bool bHalfSize = Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::SmallCards);
		_pCardList->SetCardSize(bHalfSize ? CardSize::Half : CardSize::Full);
		_pCardList->EnableTags(Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::ShowTags));
		_pGridButton->SetIcon(bHalfSize ? Resource::ICON_GRID_SMALL : Resource::ICON_GRID_LARGE);
		_pGridButton->Toggle(bHalfSize, true);
		_pToggleTagsButton->ShowBorder(Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::ShowTags));
		_pFilteringButton->ShowBorder(GetFiltering() != DefaultFilterFlags);
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
		bool bSmall = Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::SmallCards);
		bSmall = !bSmall;
		Global::GetUserSettings().SetBool(UserSetting::Interface::CharacterList::SmallCards, bSmall);

		_pGridButton->SetIcon(bSmall ? Resource::ICON_GRID_SMALL : Resource::ICON_GRID_LARGE);
		_pCardList->SetCardSize(bSmall ? CardSize::Half : CardSize::Full);
	}

	void HomeScreen::ToggleTags() noexcept
	{
		_pCardList->EnableTags(!_pCardList->IsTagsEnabled());
		Global::GetUserSettings().SetBool(UserSetting::Interface::CharacterList::ShowTags, _pCardList->IsTagsEnabled());
		_pToggleTagsButton->ShowBorder(_pCardList->IsTagsEnabled());
	}

	static bool IsShiftDown()
	{
		auto mod = SDL_GetModState();
		return (mod & SDL_KMOD_SHIFT) != 0 and (mod & SDL_KMOD_CTRL) == 0 and (mod & SDL_KMOD_ALT) == 0;
	}

	void HomeScreen::ShowSortingMenu() noexcept
	{
		auto ChangeSorting = [this](SortBy sorting) {
			Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Interface::CharacterList::Sorting, sorting);
			_pCardList->Reorder();
		};

		auto ChangeOrdering = [this](OrderBy ordering) {
			Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Interface::CharacterList::Ordering, ordering);
			_pCardList->Reorder();
		};

		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::CharacterList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::CharacterList::Ordering, OrderBy::Default);

		auto& menu = CreateMenu();
		menu.AddCheckItem("Name", sortBy == SortBy::Name)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::Name); });
		menu.AddCheckItem("Last message", sortBy == SortBy::LastUsedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::LastUsedAt); });
		menu.AddCheckItem("Chat count", sortBy == SortBy::ChatCount)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::ChatCount); });
		menu.AddCheckItem("Date created", sortBy == SortBy::CreatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::CreatedAt); });
		menu.AddCheckItem("Date modified", sortBy == SortBy::UpdatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::UpdatedAt); });
		menu.AddSeparator();
		menu.AddCheckItem("Ascending", orderBy == OrderBy::Ascending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Ascending); });
		menu.AddCheckItem("Descending", orderBy == OrderBy::Descending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Descending); });
/*		menu.AddSeparator();
		menu.AddItem("Reset")
			.SetDelegate([this] { 
				Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Interface::CharacterList::Sorting, SortBy::LastUsedAt);
				Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Interface::CharacterList::Ordering, OrderBy::Default);
				_pCardList->Reorder();
			});
		*/
		menu.Show(fig::point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void HomeScreen::ShowFilteringMenu() noexcept
	{
		auto SetFilter = [this](FilterFlags filtering) {
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, filtering, FilterFlagMapping);
			_pCardList->Reorder();
			_pFilteringButton->ShowBorder(GetFiltering() != DefaultFilterFlags);
			_pCardList->ScrollTo(0, false);
		};

		auto ToggleFilter = [this](FilterFlag flag) {
			auto filtering = GetFiltering();
			filtering.Flip(flag);
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, filtering, FilterFlagMapping);
			_pCardList->Reorder();
			_pFilteringButton->ShowBorder(GetFiltering() != DefaultFilterFlags);
			_pCardList->ScrollTo(0, false);
		};

		auto filter = GetFiltering();
		bool bShowHidden = filter.IsSet(FilterFlag::Hidden);

		auto& menu = CreateMenu();
		menu.AddCheckItem("New", filter.IsSet(FilterFlag::New))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] { 
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::Starred, FilterFlag::Chats }) | FilterFlag::New);
				else
					ToggleFilter(FilterFlag::New); 
			});
		menu.AddCheckItem("Starred", filter.IsSet(FilterFlag::Starred))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] { 
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::New, FilterFlag::Chats }) | FilterFlag::Starred);
				else
					ToggleFilter(FilterFlag::Starred); 
			});
		menu.AddCheckItem("With chats", filter.IsSet(FilterFlag::Chats))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] { 
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::New, FilterFlag::Starred}) | FilterFlag::Chats);
				else
					ToggleFilter(FilterFlag::Chats); 
			});
		menu.AddCheckItem("Hidden", bShowHidden)
			.SetDelegate([=, this] {
			ToggleFilter(FilterFlag::Hidden);
		});

		menu.AddSeparator();

		auto& genders = menu.AddItem("By gender");
		genders.AddCheckItem("Show male", filter.IsSet(FilterFlag::GenderMale))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::GenderFemale, FilterFlag::GenderOther}) | FilterFlag::GenderMale);
				else
					ToggleFilter(FilterFlag::GenderMale);
			});
		genders.AddCheckItem("Show female", filter.IsSet(FilterFlag::GenderFemale))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::GenderMale, FilterFlag::GenderOther }) | FilterFlag::GenderFemale);
				else
					ToggleFilter(FilterFlag::GenderFemale);
			});
		genders.AddCheckItem("Show non-binary", filter.IsSet(FilterFlag::GenderOther))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::GenderMale, FilterFlag::GenderFemale }) | FilterFlag::GenderOther);
				else
					ToggleFilter(FilterFlag::GenderOther);
			});

		auto& sources = menu.AddItem("By source");
		sources.AddCheckItem("Show created", filter.IsSet(FilterFlag::SourceCreated))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::SourceImported }) | FilterFlag::SourceCreated);
				else
					ToggleFilter(FilterFlag::SourceCreated); 
			});
		sources.AddCheckItem("Show imported", filter.IsSet(FilterFlag::SourceImported))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::SourceCreated }) | FilterFlag::SourceImported);
				else
					ToggleFilter(FilterFlag::SourceImported);
			});

		menu.AddSeparator();
		menu.AddItem("Clear filter")
			.SetDelegate([=, this] { 
				SetFilter(DefaultFilterFlags);
			});
		menu.Show(fig::point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}
	
}