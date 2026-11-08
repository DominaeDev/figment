#include <pch.h>
#include "gui/CardListScreen.h"
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
	template <typename T>
	CardListScreen<T>::CardListScreen(Frame* pParent, fig::string_view title) : Screen(pParent)
	{
		_pCardList = CreateControl<typename CardTypeOf<T>::ListType>();
		_pCardList->SetScrollBarOffset(0);

		auto pTopBar = CreateControl<TopBar>(title, _pCardList);

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

	template <typename T>
	void CardListScreen<T>::OnUpdate(float fElapsed)
	{
		if (_fSearchTimer > 0.0f)
		{
			if ((_fSearchTimer -= fElapsed) <= 0.0f)
				_pCardList->SetFilter(_search_text);
		}

		if (_bRefreshCardList)
			RefreshCards();
	}

	template <typename T>
	void CardListScreen<T>::OnUserSignedIn(const fig::user::UserProfile& profile)
	{
		OnActivated();

		_currentFolderId = {};

		DEBUG_MEASURE_BEGIN("CreateCards");
		_pCardList->CreateCards({});
		DEBUG_MEASURE_END();

		InvalidateLayout();
	}

	template <typename T>
	CardList<T>& CardListScreen<T>::GetCardList()
	{
		return *_pCardList;
	}

	template <typename T>
	void CardListScreen<T>::OnSearchFilter(fig::string_view search_text)
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

	template <typename T>
	void CardListScreen<T>::ToggleCardSize() noexcept
	{
		bool bSmall = Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::SmallCards);
		bSmall = !bSmall;
		Global::GetUserSettings().SetBool(UserSetting::Interface::CharacterList::SmallCards, bSmall);

		_pGridButton->SetIcon(bSmall ? Icon::GRID_SMALL : Icon::GRID_LARGE);
		_pCardList->SetCardSize(bSmall ? CardSize::Half : CardSize::Full);
	}

	template <typename T>
	void CardListScreen<T>::ToggleTags() noexcept
	{
		_pCardList->EnableTags(!_pCardList->IsTagsEnabled());
		Global::GetUserSettings().SetBool(UserSetting::Interface::CharacterList::ShowTags, _pCardList->IsTagsEnabled());
		RefreshTopButtons();
	}

	template <typename T>
	void CardListScreen<T>::OnActivated()
	{
		RefreshTopButtons();
		RefreshCards();
	}

	template <typename T>
	void CardListScreen<T>::RefreshCards()
	{
		_pCardList->SetCardSize(Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::SmallCards) ? CardSize::Half : CardSize::Full);
		_pCardList->EnableTags(Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::ShowTags));
		_pCardList->RefreshCards();
		_bRefreshCardList = false;
	}

	template <typename T>
	void CardListScreen<T>::RefreshTopButtons()
	{
		if (not Global::IsSignedIn())
			return;

		bool bHalfSize = Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::SmallCards);
		_pGridButton->SetIcon(bHalfSize ? Icon::GRID_SMALL : Icon::GRID_LARGE);
		_pGridButton->SetOn(bHalfSize, true);

		auto [_, bSameAsDefault] = GetFiltering();
		if (not bSameAsDefault)
			_pFilteringButton->SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8)->SetColor(Color::BorderSelected);
		else
			_pFilteringButton->ClearBorderRenderer();

		bool bShowTags = Global::GetUserSettings().GetBool(UserSetting::Interface::CharacterList::ShowTags);
		_pToggleTagsButton->SetOn(bShowTags, true);
	}

	template <typename T>
	bool CardListScreen<T>::ChangeFolder(const fig::uuid& folderId)
	{
		if (folderId == _currentFolderId)
			return false;

		_currentFolderId = folderId;
		_pCardList->Clear();
		_pCardList->CreateCards(_currentFolderId);
		return true;
	}

}