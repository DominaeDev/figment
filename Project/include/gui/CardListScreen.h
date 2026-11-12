#pragma once

#include "gui/Screen.h"
#include "gui/GUICommon.h"
#include "gui/CardList.h"

namespace fig::gui
{
	class ToggleWithIcon;
	class FolderWidget;

	template <typename T>
	class CardListScreen : public Screen
	{
		using TCardList = CardList<T>;
	public:
		CardListScreen(Frame* pParent, fig::string_view title);

		bool ChangeFolder(const fig::uuid& folderId);
		void RefreshCards();
		TCardList& GetCardList();
		const fig::uuid& GetCurrentFolder() const noexcept { return _currentFolderId; }

	protected:
		void OnUpdate(float fElapsed) override;
		virtual void ShowSortingMenu() noexcept = 0;
		virtual void ShowFilteringMenu() noexcept = 0;
		virtual std::pair<fig::io::FilterFlags, bool> GetFiltering() const noexcept = 0;

		void OnSearchFilter(fig::string_view search_text);
		void OnActivated() override;
		void OnUserSignedIn(const fig::user::UserProfile& profile) override;
		void OnChangedFolder(const fig::uuid& folderId);
		EventResult OnEvent(fig::event& event) override;

		void ToggleCardSize() noexcept;
		void ToggleTags() noexcept;
		void RefreshTopButtons();

		fig::observer_ptr<TCardList> _pCardList;
		fig::observer_ptr<TextInput> _pFilterTextBox;
		fig::observer_ptr<StaticText> _pHeader;
		fig::observer_ptr<ButtonWithIcon> _pSortingButton;
		fig::observer_ptr<ButtonWithIcon> _pFilteringButton;
		fig::observer_ptr<Control> _pFilterBorder;
		fig::observer_ptr<ToggleWithIcon> _pToggleTagsButton;
		fig::observer_ptr<ToggleWithIcon> _pGridButton;
		fig::observer_ptr<FolderWidget> _pFolders;

		fig::string _search_text;
		float _fSearchTimer {};
		bool _bRefreshCardList {};

		fig::uuid _currentFolderId {};
	};
}
