#include <pch.h>
#include "gui/AppResources.h"
#include "gui/ChatListingScreen.h"
#include "gui/ChatList.h"
#include "gui/SearchBox.h"
#include "gui/MainFrame.h"
#include "gui/Menu.h"
#include "app/AppState.h"
#include "user/UserManager.h"

namespace fig::gui
{
	static ChatFilterFlags GetFiltering()
	{
		return Global::GetUserSettings().GetFlags<ChatFilterFlags>(UserSetting::ChatList_Filtering, DefaultChatFilterFlags, ChatFilterFlagMapping);
	};

	ChatListingScreen::ChatListingScreen(Frame* pParent) : Screen(pParent)
	{
		auto pTopBar = CreateControl<Panel>();
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto _pHeader = pTopBar->CreateControl<StaticText>("Chats", FontFace::Italic, 24, false);
		_pHeader->SetHeight(Constants::GUI::SidePanel::HeaderHeight);
		_pHeader->SetAlignment(TextAlignment::Left_Center);

		_pSortingButton = pTopBar->CreateControl<ButtonWithIcon>(Resource::ICON_SORTING);
		_pSortingButton->SetDelegate([this]() { ShowSortingMenu(); });

		_pFilteringButton = pTopBar->CreateControl<ButtonWithIcon>(Resource::ICON_FILTERING);
		_pFilteringButton->SetDelegate([this]() { ShowFilteringMenu(); });

		auto _pFilterTextBox = pTopBar->CreateControl<SearchBox>(FontFace::Default, 16.0);
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(192, 30);
		_pFilterTextBox->SetMaxSize(192, -1);
		_pFilterTextBox->SetBackgroundColor(Colors::White);
		_pFilterTextBox->SetTextChangedCallback([this](fig::string s) {
			OnSearchFilter(s);
		});

		auto pTopSizer = pTopBar->SetSizer<HorizontalSizer>();
		pTopSizer->AddSpacer(12);
		pTopSizer->Add(_pHeader, 0, Sizer::AlignCenterVertical | Sizer::Left, 6);
		pTopSizer->AddStretchSpacer();
		pTopSizer->Add(_pSortingButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 2);
		pTopSizer->Add(_pFilteringButton, 0, Sizer::AlignCenterVertical | Sizer::Right, 8);
		pTopSizer->Add(_pFilterTextBox, 0, Sizer::AlignCenterVertical | Sizer::Right, 8);

		_pChatList = CreateControl<ChatList>();

		auto mainSizer = SetSizer<VerticalSizer>();
		mainSizer->Add(pTopBar, 0, Sizer::Expand);
		mainSizer->Add(_pChatList, -1, Sizer::Fill | Sizer::Left, 12);

		InvalidateLayout();
	}

	void ChatListingScreen::OnUpdate(float fElapsed)
	{
		if (_fSearchTimer > 0.0f)
		{
			if ((_fSearchTimer -= fElapsed) <= 0.0f)
				_pChatList->SetFilter(_search_text);
		}
	}

	void ChatListingScreen::ShowAllChats()
	{
		if (not Global::IsSignedIn())
			return;

		_pChatList->ShowAllChats();
		_filterByCharacter.clear();
		RefreshFilterButton();
	}

	void ChatListingScreen::ShowChatsWith(const fig::uuid& characterId)
	{
		if (not Global::IsSignedIn())
			return;

		_filterByCharacter = Global::GetUserContent().GetCharacterName(characterId).value_or("Unknown");
		_pChatList->ShowChatsWith(characterId);
		RefreshFilterButton();
	}

	bool ChatListingScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		return false;
	}

	EventResult ChatListingScreen::OnEvent(fig::event& event)
	{
		// ...

		return Screen::OnEvent(event);
	}

	void ChatListingScreen::ShowSortingMenu() noexcept
	{
		auto ChangeSorting = [this](SortBy sorting) {
			Global::GetUserSettings().SetEnum<SortBy>(UserSetting::ChatList_Sorting, sorting);
			_pChatList->Reorder();
		};

		auto ChangeOrdering = [this](OrderBy ordering) {
			Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::ChatList_Ordering, ordering);
			_pChatList->Reorder();
		};

		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::ChatList_Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::ChatList_Ordering, OrderBy::Default);

		auto& menu = MainFrame::GetInstance().CreateMenu();
		menu.AddCheckItem("Sort by creation date", sortBy == SortBy::CreatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::CreatedAt); });
		menu.AddCheckItem("Sort by modified date", sortBy == SortBy::UpdatedAt)
			.SetDelegate([ChangeSorting, this] { ChangeSorting(SortBy::UpdatedAt); });
		menu.AddSeparator();
		menu.AddCheckItem("Ascending", orderBy == OrderBy::Ascending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Ascending); });
		menu.AddCheckItem("Descending", orderBy == OrderBy::Descending)
			.SetDelegate([ChangeOrdering, this] { ChangeOrdering(OrderBy::Descending); });
		menu.AddSeparator();
		menu.AddItem("Reset")
			.SetDelegate([this] {
				Global::GetUserSettings().SetEnum<SortBy>(UserSetting::ChatList_Sorting, SortBy::Default);
				Global::GetUserSettings().SetEnum<OrderBy>(UserSetting::ChatList_Ordering, OrderBy::Default);
				_pChatList->Reorder();
			});

		menu.Show(fig::point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void ChatListingScreen::ShowFilteringMenu() noexcept
	{
		auto SetFilter = [this](ChatFilterFlags filtering) {
			Global::GetUserSettings().SetFlags<ChatFilterFlags>(UserSetting::ChatList_Filtering, filtering, ChatFilterFlagMapping);
			_pChatList->Reorder();
			_pChatList->ScrollTo(0, false);
			RefreshFilterButton();
		};

		auto ToggleFilter = [this](ChatFilterFlag flag) {
			auto filtering = GetFiltering();
			filtering.Flip(flag);
			Global::GetUserSettings().SetFlags<ChatFilterFlag>(UserSetting::ChatList_Filtering, filtering, ChatFilterFlagMapping);
			_pChatList->Reorder();
			_pChatList->ScrollTo(0, false);
			RefreshFilterButton();
		};

		auto filter = GetFiltering();
		bool bShowHidden = filter.IsSet(ChatFilterFlag::Hidden);

		auto& menu = MainFrame::GetInstance().CreateMenu();

		if (not _filterByCharacter.empty())
		{
			menu.AddCheckItem(std::format("Character: {}", _filterByCharacter), true)
				.SetDelegate([this] { ShowAllChats(); });
			menu.AddSeparator();
		}

		menu.AddCheckItem("Starred", filter.IsSet(ChatFilterFlag::Starred))
			.SetEnabled(!bShowHidden)
			.SetDelegate([=, this] {
				ToggleFilter(ChatFilterFlag::Starred);
			});
		menu.AddCheckItem("Archived", bShowHidden)
			.SetDelegate([=, this] {
				ToggleFilter(ChatFilterFlag::Hidden);
			});

		menu.AddSeparator();

		menu.AddItem("Clear filter")
			.SetDelegate([=, this] {
				SetFilter(DefaultChatFilterFlags);
				_filterByCharacter.clear();
				ShowAllChats();
			});
		menu.Show(fig::point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}

	void ChatListingScreen::OnSearchFilter(fig::string search_text)
	{
		if (search_text.size() < 2)
		{
			_pChatList->SetFilter("");
			_search_text.clear();
			_fSearchTimer = 0.0f;
		}
		else
		{
			_search_text = std::move(search_text);
			_fSearchTimer = 0.005f;
		}
	}

	void ChatListingScreen::RefreshFilterButton()
	{
		_pFilteringButton->EnableBorder((GetFiltering() != DefaultChatFilterFlags) or !_filterByCharacter.empty());
	}
}