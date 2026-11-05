#include <pch.h>
#include "gui/StoryCardList.h"
#include "gui/StoryScreen.h"
#include "gui/CardListScreen.impl.h"

namespace fig::gui
{
	StoryScreen::StoryScreen(Frame* pParent) : CardListScreen(pParent, "Stories")
	{
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
			auto [filter, _] = GetFiltering();
			filter.Flip(flag);
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, filter, FilterFlagMapping);
			_pCardList->Reorder();
			RefreshTopButtons();
			_pCardList->ScrollTo(0, false);
		};

		auto [filter, _] = GetFiltering();
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
				SetFilter(DefaultScenarioFilterFlags);
			});
		menu.Show(fig::point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}

	std::pair<fig::io::FilterFlags, bool> StoryScreen::GetFiltering() const noexcept
	{
		FilterFlags flags = DefaultScenarioFilterFlags;
		if (Global::IsSignedIn())
			flags = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, DefaultScenarioFilterFlags, FilterFlagMapping);
		
		return std::pair { flags, flags == DefaultScenarioFilterFlags };
	}

	template class CardListScreen<fig::data::Scenario>;

}