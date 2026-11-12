#include <pch.h>
#include "gui/CharacterCardList.h"
#include "gui/CharacterScreen.h"
#include "gui/CardListScreen.impl.h"

namespace fig::gui
{
	CharacterScreen::CharacterScreen(Frame* pParent) : CardListScreen(pParent, "Characters")
	{
	}

	EventResult CharacterScreen::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::AddedCharacter))
		{
			_bRefreshCardList = true;
			return EventResult::Continue;
		}

		return CardListScreen<fig::data::Character>::OnEvent(event);
	}

	void CharacterScreen::ShowSortingMenu() noexcept
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
		menu.Show(fig::point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void CharacterScreen::ShowFilteringMenu() noexcept
	{
		auto SetFilter = [this](FilterFlags filtering) {
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, filtering, FilterFlagMapping);
			_pCardList->Reorder();
			RefreshTopButtons();
			_pCardList->ScrollTo(0, false);
		};

		auto ToggleFilter = [this](FilterFlag flag) {
			auto [filter, _] = GetFiltering();
			filter.Flip(flag);
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, filter, FilterFlagMapping);
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
		menu.AddCheckItem("With chats", filter.IsSet(FilterFlag::Chats))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] { 
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::New, FilterFlag::Starred, FilterFlag::Voiced }) | FilterFlag::Chats);
				else
					ToggleFilter(FilterFlag::Chats); 
			});
		menu.AddCheckItem("With voice", filter.IsSet(FilterFlag::Voiced))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] { 
				if (IsShiftDown())
					SetFilter((filter & ~FilterFlags { FilterFlag::New, FilterFlag::Starred , FilterFlag::Chats }) | FilterFlag::Voiced);
				else
					ToggleFilter(FilterFlag::Voiced); 
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
				SetFilter(DefaultCharacterFilterFlags);
			});
		menu.Show(fig::point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}
	
	std::pair<fig::io::FilterFlags, bool> CharacterScreen::GetFiltering() const noexcept
	{
		FilterFlags flags = DefaultCharacterFilterFlags;
		if (Global::IsSignedIn())
			flags = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::CharacterList::Filtering, DefaultCharacterFilterFlags, FilterFlagMapping);
		
		return std::pair { flags, flags == DefaultCharacterFilterFlags };
	}

	template class CardListScreen<fig::data::Character>;
}