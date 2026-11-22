#include <pch.h>
#include "gui/UserPersonaScreen.h"
#include "gui/AppResources.h"
#include "gui/UserPersonaList.h"
#include "gui/SearchBox.h"
#include "gui/Menu.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/ButtonWithIcon.h"
#include "app/AppState.h"
#include "user/UserManager.h"

using namespace fig::io;

namespace fig::gui
{
	static FilterFlags GetFiltering()
	{
		return Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::UserPersonaList::Filtering, DefaultUserPersonaFilterFlags, FilterFlagMapping);
	};

	UserPersonaScreen::UserPersonaScreen(Frame* pParent) : Screen(pParent)
	{
		auto pTopBar = CreateControl<Panel>();
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto _pHeader = pTopBar->CreateControl<StaticText>("Identities", FontFace::Italic, 24, false);
		_pHeader->SetHeight(Constants::GUI::SidePanel::HeaderHeight);
		_pHeader->SetAlignment(TextAlignment::LeftCenter);

		_pSortingButton = pTopBar->CreateControl<ButtonWithIcon>(Icon::SORTING, false);
		_pSortingButton->SetTheme(ButtonThemes::TopBar);
		_pSortingButton->SetDelegate([this]() { ShowSortingMenu(); });

		_pFilteringButton = pTopBar->CreateControl<ButtonWithIcon>(Icon::FILTERING, true);
		_pFilteringButton->SetTheme(ButtonThemes::TopBar);
		_pFilteringButton->SetDelegate([this]() { ShowFilteringMenu(); });

		auto _pFilterTextBox = pTopBar->CreateControl<SearchBox>();
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(192, 30);
		_pFilterTextBox->SetMaxWidth(192);
		_pFilterTextBox->SetTextChangedDelegate([this](fig::string_view s) {
			OnSearchFilter(s);
		});

		auto pTopSizer = pTopBar->SetSizer<HorizontalSizer>();
		pTopSizer->Add(_pHeader, 0, SizerFlag::AlignCenterVertical | SizerFlag::Left, 18);
		pTopSizer->AddStretchSpacer();
		pTopSizer->Add(_pSortingButton, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 2);
		pTopSizer->Add(_pFilteringButton, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 8);
		pTopSizer->Add(_pFilterTextBox, 0, SizerFlag::AlignCenterVertical | SizerFlag::Right, 8);

		_pUserPersonaList = CreateControl<UserPersonaList>();

		auto mainSizer = SetSizer<VerticalSizer>();
		mainSizer->Add(pTopBar, 0, SizerFlag::Expand);
		mainSizer->Add(_pUserPersonaList, -1, SizerFlag::Fill | SizerFlag::Left, 12);

		InvalidateLayout();
	}

	void UserPersonaScreen::OnUpdate(float fElapsed)
	{
		if (_fSearchTimer > 0.0f)
		{
			if ((_fSearchTimer -= fElapsed) <= 0.0f)
				_pUserPersonaList->SetFilter(_search_text);
		}
	}

	void UserPersonaScreen::Initialize()
	{
		if (not Global::IsSignedIn())
			return;

		_pUserPersonaList->CreateCards();
		RefreshFilterButton();
	}

	void UserPersonaScreen::ShowSortingMenu() noexcept
	{
		auto ChangeSorting = [this](SortBy sorting) {
			Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Interface::UserPersonaList::Sorting, sorting);
			_pUserPersonaList->Reorder();
		};

		auto ChangeOrdering = [this](OrderBy ordering) {
			Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Interface::UserPersonaList::Ordering, ordering);
			_pUserPersonaList->Reorder();
		};

		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::UserPersonaList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::UserPersonaList::Ordering, OrderBy::Default);

		auto& menu = CreateMenu();
		menu.AddCheckItem("Name", sortBy == SortBy::Name)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::Name); });
		menu.AddCheckItem("Last message", sortBy == SortBy::LastUsedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::LastUsedAt); });
		menu.AddCheckItem("Date created", sortBy == SortBy::CreatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::CreatedAt); });
		menu.AddCheckItem("Date modified", sortBy == SortBy::UpdatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::UpdatedAt); });
		menu.AddSeparator();
		menu.AddCheckItem("Ascending", orderBy == OrderBy::Ascending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Ascending); });
		menu.AddCheckItem("Descending", orderBy == OrderBy::Descending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Descending); });
		menu.AddSeparator();
		menu.AddItem("Reset")
			.SetDelegate([this] {
				Global::GetUserSettings().SetEnum<SortBy>(UserSetting::Interface::UserPersonaList::Sorting, SortBy::Default);
				Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::Interface::UserPersonaList::Ordering, OrderBy::Default);
				_pUserPersonaList->Reorder();
			});

		menu.Show(fig::point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void UserPersonaScreen::ShowFilteringMenu() noexcept
	{
		auto SetFilter = [this](FilterFlags filtering) {
			Global::GetUserSettings().SetFlags<FilterFlags>(UserSetting::Interface::UserPersonaList::Filtering, filtering, FilterFlagMapping);
			_pUserPersonaList->Reorder();
			_pUserPersonaList->ScrollTo(0, false);
			RefreshFilterButton();
		};

		auto ToggleFilter = [this](FilterFlag flag) {
			auto filtering = GetFiltering();
			filtering.Flip(flag);
			Global::GetUserSettings().SetFlags<FilterFlag>(UserSetting::Interface::UserPersonaList::Filtering, filtering, FilterFlagMapping);
			_pUserPersonaList->Reorder();
			_pUserPersonaList->ScrollTo(0, false);
			RefreshFilterButton();
		};

		auto filter = GetFiltering();
		bool bShowHidden = filter.IsSet(FilterFlag::Hidden);

		auto& menu = CreateMenu();
		menu.AddCheckItem("New", filter.IsSet(FilterFlag::New))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] { 
				ToggleFilter(FilterFlag::New); 
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
		menu.AddSeparator();

		menu.AddItem("Clear filter")
			.SetDelegate([=, this] {
				SetFilter(DefaultUserPersonaFilterFlags);
			});
		menu.Show(fig::point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}

	void UserPersonaScreen::OnSearchFilter(fig::string_view search_text)
	{
		if (search_text.size() < 2)
		{
			_pUserPersonaList->SetFilter("");
			_search_text.clear();
			_fSearchTimer = 0.0f;
		}
		else
		{
			_search_text = fig::string { search_text };
			_fSearchTimer = 0.005f;
		}
	}

	void UserPersonaScreen::RefreshFilterButton()
	{
		if (GetFiltering() != DefaultUserPersonaFilterFlags)
			_pFilteringButton->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8)->SetColor(Color::BorderSelected);
		else
			_pFilteringButton->ClearBorderRenderer();
	}

	EventResult UserPersonaScreen::OnEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::UserSettingsChanged))
		{
			_pUserPersonaList->Reorder();
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}
}