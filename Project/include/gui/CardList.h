#pragma once

#include "gui/ScrollPanel.h"
#include "gui/CoverCard.h"
#include "gui/CardTypes.h"
#include "io/AssetFolder.h"
#include "user/UserSettings.h"

namespace fig::gui
{
	class GridSizer;
	class Menu;
	class MenuItem;

	template <typename T>
	class CardList : public ScrollPanel
	{
	protected:
		using TCard = CardTypeOf<T>::Type;
		using CardPtr = fig::observer_ptr<TCard>;
	public:
		CardList(ControlPtr pParent, CardSize cardSize = CardSize::Full);

		void CreateCards(const fig::uuid& folderId = {});
		void RefreshCards();
		void Clear();

		void SetFilter(const fig::string& filter) noexcept;
		void Reorder();

		void SetCardSize(CardSize cardSize);
		void EnableTags(bool bEnable) noexcept;
		inline bool IsTagsEnabled() const noexcept { return _bEnableTags; }

		const fig::uuid& GetCurrentFolderId() const noexcept { return _currentFolderId; }

	protected:
		virtual std::tuple<fig::io::SortBy, fig::io::OrderBy, fig::io::FilterFlags> GetSortOrder() = 0;

		void OnUpdate(float fElapsed) override;
		void OnScroll() override;
		fig::coord GetExtent() const override;

		virtual uint32_t OnCardMenu(CardPtr card) { return (uint32_t)-1; };
		void OnModifiedCard(CardPtr pCard);
		void OnDeleteCard(CardPtr pCard);

		void CreateMoveMenu(Menu& menu, const fig::uuid& assetId, fig::string_view rootName);
		void CreateMoveSubMenu(MenuItem& parentItem, const fig::uuid& assetId, const fig::uuid& folderId);
		void MoveCardToFolder(const fig::uuid& assetId, const fig::uuid& folderId);

		bool _bInitialized { false };
		std::vector<CardPtr> _cards;
		fig::uuid _currentFolderId {};

		fig::observer_ptr<GridSizer> _pGridSizer;
		int32_t _last_rows {};
		CardSize _cardSize;
		fig::string _filterString;
		bool _bEnableTags { false };

	};
}
