#include <pch.h>
#include "gui/StoryScreen.h"
#include "gui/StoryCardList.h"
#include "gui/SearchBox.h"
#include "gui/AppResources.h"
#include "gui/Menu.h"
#include "gui/ToggleWithIcon.h"
#include "gui/TexturedBorder.h"
#include "gui/ButtonWithIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/TopBar.h"
#include "app/AppState.h"
#include "user/UserManager.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	static FilterFlags GetFiltering()
	{
		if (Global::IsSignedIn())
			return Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, DefaultFilterFlags, FilterFlagMapping);
		else
			return DefaultFilterFlags;
	};

	StoryScreen::StoryScreen(Frame* pParent) : Screen(pParent)
	{
		_pCardList = CreateControl<StoryCardList>();
		_pCardList->SetScrollBarOffset(0);

		auto pTopBar = CreateControl<TopBar>("Stories", _pCardList);

		_pSortingButton = pTopBar->CreateControl<ButtonWithIcon>(Icon::SORTING, false);
		_pSortingButton->SetTheme(ButtonThemes::TopBar);
		_pSortingButton->SetDelegate([this]() { ShowSortingMenu(); });

		_pGridButton = pTopBar->CreateControl<ToggleWithIcon>(Icon::GRID_LARGE);
		_pGridButton->SetTheme(ButtonThemes::TopBar);
		_pGridButton->SetDelegate([this](bool _) { ToggleCardSize(); });

		_pFilteringButton = pTopBar->CreateControl<ButtonWithIcon>(Icon::FILTERING, true);
		_pFilteringButton->SetTheme(ButtonThemes::TopBar);
		_pFilteringButton->SetDelegate([this]() { ShowFilteringMenu(); });

		_pToggleTagsButton = pTopBar->CreateControl<ToggleWithIcon>(Icon::TAG);
		_pToggleTagsButton->SetTheme(ButtonThemes::TopBar);
		_pToggleTagsButton->SetDelegate([this](bool _) { ToggleTags(); });

		_pFilterTextBox = pTopBar->CreateControl<SearchBox>();
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(192, 30);
		_pFilterTextBox->SetMaxWidth(192);
		_pFilterTextBox->SetTextChangedDelegate([this](fig::string_view s) {
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

	void StoryScreen::OnUpdate(float fElapsed)
	{
		if (_fSearchTimer > 0.0f)
		{
			if ((_fSearchTimer -= fElapsed) <= 0.0f)
				_pCardList->SetFilter(_search_text);
		}

		if (_bRefreshCardList)
			RefreshCards();
	}

	void StoryScreen::OnRender(fig::renderer_ptr pRenderer)
	{
		DrawBackground(pRenderer);
	}

	EventResult StoryScreen::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::AddedStory))
		{
			_bRefreshCardList = true;
			return EventResult::Continue;
		}

		return Screen::OnEvent(event);
	}

	bool StoryScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		if (event.pressed)
		{
		}
		else // Released
		{
		}
		return false;
	}

	void StoryScreen::OnUserSignedIn(const fig::user::UserProfile& profile)
	{
		bool bHalfSize = Global::GetUserSettings().GetBool(UserSetting::Interface::ScenarioList::SmallCards);
		_pCardList->SetCardSize(bHalfSize ? CardSize::Half : CardSize::Full);
		_pCardList->EnableTags(Global::GetUserSettings().GetBool(UserSetting::Interface::ScenarioList::ShowTags));
		_pGridButton->SetIcon(bHalfSize ? Icon::GRID_SMALL : Icon::GRID_LARGE);
		_pGridButton->SetOn(bHalfSize, true);
		RefreshTopButtons();

		// Create cards
		DEBUG_MEASURE_BEGIN("CreateCards");
		_pCardList->Clear();
		_pCardList->CreateCards();
		DEBUG_MEASURE_END();

		InvalidateLayout();

	}

	StoryCardList& StoryScreen::GetCardList()
	{
		return *_pCardList;
	}

	void StoryScreen::OnSearchFilter(fig::string_view search_text)
	{
		if (search_text.size() < 2)
		{
			_pCardList->SetFilter("");
			_search_text.clear();
			_fSearchTimer = 0.0f;
		}
		else
		{
			_search_text = fig::string { search_text };
			_fSearchTimer = 0.005f;
		}
	}

	void StoryScreen::ToggleCardSize() noexcept
	{
		bool bSmall = Global::GetUserSettings().GetBool(UserSetting::Interface::ScenarioList::SmallCards);
		bSmall = !bSmall;
		Global::GetUserSettings().SetBool(UserSetting::Interface::ScenarioList::SmallCards, bSmall);

		_pGridButton->SetIcon(bSmall ? Icon::GRID_SMALL : Icon::GRID_LARGE);
		_pCardList->SetCardSize(bSmall ? CardSize::Half : CardSize::Full);
	}

	void StoryScreen::ToggleTags() noexcept
	{
		_pCardList->EnableTags(!_pCardList->IsTagsEnabled());
		Global::GetUserSettings().SetBool(UserSetting::Interface::ScenarioList::ShowTags, _pCardList->IsTagsEnabled());
		RefreshTopButtons();
	}

	void StoryScreen::ShowSortingMenu() noexcept
	{
		auto ChangeSorting = [this](SortBy sorting) {
			Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Interface::ScenarioList::Sorting, sorting);
			_pCardList->Reorder();
		};

		auto ChangeOrdering = [this](OrderBy ordering) {
			Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Interface::ScenarioList::Ordering, ordering);
			_pCardList->Reorder();
		};

		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::ScenarioList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::ScenarioList::Ordering, OrderBy::Default);

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
		menu.Show(fig::point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void StoryScreen::ShowFilteringMenu() noexcept
	{
		auto SetFilter = [this](FilterFlags filtering) {
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, filtering, FilterFlagMapping);
			_pCardList->Reorder();
			RefreshTopButtons();
			_pCardList->ScrollTo(0, false);
		};

		auto ToggleFilter = [this](FilterFlag flag) {
			auto filtering = GetFiltering();
			filtering.Flip(flag);
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, filtering, FilterFlagMapping);
			_pCardList->Reorder();
			RefreshTopButtons();
			_pCardList->ScrollTo(0, false);
		};

		auto filter = GetFiltering();
		bool bShowHidden = filter.IsSet(FilterFlag::Hidden);

		auto& menu = CreateMenu();
		menu.AddCheckItem("New", filter.IsSet(FilterFlag::New))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::Starred, FilterFlag::Chats, FilterFlag::Voiced }) | FilterFlag::New);
				else
					ToggleFilter(FilterFlag::New);
			});
		menu.AddCheckItem("Starred", filter.IsSet(FilterFlag::Starred))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::New, FilterFlag::Chats, FilterFlag::Voiced }) | FilterFlag::Starred);
				else
					ToggleFilter(FilterFlag::Starred);
			});
		menu.AddCheckItem("Hidden", bShowHidden)
			.SetDelegate([=, this] {
				ToggleFilter(FilterFlag::Hidden);
			});

		menu.AddSeparator();

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

	void StoryScreen::OnActivated()
	{
		RefreshCards();
	}

	void StoryScreen::RefreshCards()
	{
		_pCardList->RefreshCards();
		_bRefreshCardList = false;
	}

	void StoryScreen::RefreshTopButtons()
	{
		if (not Global::IsSignedIn())
			return;

		if (GetFiltering() != DefaultFilterFlags)
			_pFilteringButton->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8)->SetColor(Color::BorderSelected);
		else
			_pFilteringButton->ClearBorderRenderer();

		bool bShowTags = Global::GetUserSettings().GetBool(UserSetting::Interface::ScenarioList::ShowTags);
		_pToggleTagsButton->SetOn(bShowTags, true);
	}

}