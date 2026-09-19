#pragma once

#include "gui/ScrollPanel.h"
#include "gui/CharacterCard.h"
#include "user/UserSettings.h"

namespace fig::gui
{
	class GridSizer;

	class CharacterCardList : public ScrollPanel
	{
	public:
		CharacterCardList(ControlPtr pParent, CardSize cardSize = CardSize::Full);

		void CreateCards();
		void Clear();

		void SetFilter(const fig::string& filter) noexcept;
		void Reorder();

		void RefreshCards();

		void SetCardSize(CardSize cardSize);
		void EnableTags(bool bEnable) noexcept;
		inline bool IsTagsEnabled() const noexcept { return _bEnableTags; }

	protected:
		void OnUpdate(float fElapsed) override;
		void OnScroll() override;
		fig::coord GetExtent() const override;

		void OnCardEvent(CoverCard& card, CardEvent event);
		void DeleteCharacter(CoverCard& card);

	private:
		bool _bInitialized { false };
		std::vector<fig::observer_ptr<CharacterCard>> _cards;

		fig::observer_ptr<GridSizer> _pGridSizer;
		int32_t _last_rows {};
		CardSize _cardSize;
		fig::string _filterString;
		bool _bEnableTags { false };
	};
}
