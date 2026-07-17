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
	ChatListingScreen::ChatListingScreen(Frame* pParent) : Screen(pParent)
	{
		auto pTopBar = CreateControl<Panel>();
		pTopBar->SetHeight(Constants::GUI::SidePanel::HeaderHeight);

		auto _pHeader = pTopBar->CreateControl<StaticText>("Chats", FontFace::Italic, 24, false);
		_pHeader->SetHeight(Constants::GUI::SidePanel::HeaderHeight);
		_pHeader->SetAlignment(TextAlignment::Left_Center);

		_pSortingButton = pTopBar->CreateControl<ButtonWithIcon>(TextureType::ICON_SORTING);
		_pSortingButton->SetDelegate([this]() { ShowSortingMenu(); });

		_pFilteringButton = pTopBar->CreateControl<ButtonWithIcon>(TextureType::ICON_FILTERING);
		_pFilteringButton->SetDelegate([this]() { ShowFilteringMenu(); });

		auto _pFilterTextBox = pTopBar->CreateControl<SearchBox>(FontFace::Default, 16.0);
		_pFilterTextBox->SetPosition(0, 0);
		_pFilterTextBox->SetSize(192, 30);
		_pFilterTextBox->SetMaxSize(192, -1);
		_pFilterTextBox->SetBackgroundColor(Colors::White);
		_pFilterTextBox->SetTextChangedCallback([this](fig::string s) {
//			this->OnSearchFilter(s);
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

	void ChatListingScreen::ShowAllChats()
	{
		if (not Global::IsSignedIn())
			return;

		_pChatList->ShowAllChats();
	}

	void ChatListingScreen::ShowChatsWith(const fig::uuid& characterId)
	{
		if (not Global::IsSignedIn())
			return;

		_pChatList->ShowChatsWith(characterId);
	}

	bool ChatListingScreen::OnKeyboardEvent(KeyboardEvent& event)
	{
		return false;
	}

	EventResult ChatListingScreen::OnEvent(Event& event)
	{
		// ...

		return Screen::OnEvent(event);
	}

	void ChatListingScreen::ShowSortingMenu() noexcept
	{
		auto& menu = MainFrame::GetInstance().CreateMenu();
		menu.AddCheckItem("Sort by creation date");
		menu.AddCheckItem("Sort by modified date");
		menu.AddSeparator();
		menu.AddCheckItem("Ascending");
		menu.AddCheckItem("Descending");
		menu.AddSeparator();
		menu.AddItem("Reset");

		menu.Show(Point { _pSortingButton->GetAbsoluteX(), _pSortingButton->GetAbsoluteY() + _pSortingButton->GetHeight() });
	}

	void ChatListingScreen::ShowFilteringMenu() noexcept
	{
		auto& menu = MainFrame::GetInstance().CreateMenu();
		menu.AddCheckItem("New");
		menu.AddCheckItem("Starred");
		menu.AddCheckItem("Archived");
		menu.AddSeparator();
		menu.AddItem("Clear filter");
		menu.Show(Point { _pFilteringButton->GetAbsoluteX(), _pFilteringButton->GetAbsoluteY() + _pFilteringButton->GetHeight() });
	}

}