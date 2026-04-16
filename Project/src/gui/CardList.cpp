#include <pch.h>
#include "model/AppState.h"
#include "model/UserManager.h"
#include "model/AssetManager.h"
#include "util/Common.h"
#include "util/StringUtility.h"
#include "gui/CardList.h"
#include "gui/GridSizer.h"
#include "gui/ScenarioCard.h"
#include "gui/CharacterCard.h"
#include "gui/MainFrame.h"

using namespace fig::io;
using namespace fig::util;

namespace fig::gui
{
	constexpr Coord TopMargin = 8;
	constexpr Coord BottomMargin = 120;

	static constexpr Coord cardWidth(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Full ? Constants::GUI::CardWidth : Constants::GUI::HalfCardWidth;;
	}

	static constexpr Coord cardHeight(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Full ? Constants::GUI::CardHeight: Constants::GUI::HalfCardHeight;
	}

	CardList::CardList(LayoutElement* pParent, CardSize cardSize) : ScrollPanel(pParent),
		_cardSize { cardSize }
	{
		_pGridSizer = new GridSizer(cardWidth(cardSize), cardHeight(cardSize));
		_pGridSizer->SetSpacing(Constants::GUI::CardSpacingX, Constants::GUI::CardSpacingY);
		_pGridSizer->EnableCentering(true);
		SetTopMargin(TopMargin);
		SetBottomMargin(BottomMargin);

		SetSizer(_pGridSizer);
		EnableClipping(true);
		EnableCulling(true);
	}

	void CardList::CreateCards(CardType cardType)
	{
		// Create cards
		auto& assets = Global::GetUserManager().GetProfileAssets();
		const auto& profileId = Global::GetUserManager().GetActiveProfile().id;

		Reset();

		int32_t priority = 0;
		if (cardType == CardType::Scenario)
		{
			// Find scenarios
			auto scenarios = assets.GetAllScenarios() | std::ranges::to<std::vector>();
			std::sort(scenarios.begin(), scenarios.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : scenarios)
			{
				DEBUG_MEASURE_BEGIN(std::format("Scenario card {}:", asset.id.str()));
				auto pCard = new ScenarioCard(this, asset.id, _cardSize);
				auto request = assets.LoadAssetAsync(asset.id, AsyncTask::LoadCoverImage, priority--);
				pCard->SetPendingCoverImage(std::move(request.future));
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
				DEBUG_MEASURE_END();
			}
		}

		// Find characters
		if (cardType == CardType::Character)
		{
			auto characters = assets.GetAllCharacters() | std::ranges::to<std::vector>();
			std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : characters)
			{
				DEBUG_MEASURE_BEGIN(std::format("Character card {}:", asset.id.str()));
				auto pCard = new CharacterCard(this, asset.id, _cardSize);
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
				DEBUG_MEASURE_END();
			}
		}

		Reorder();

		size_t initCounter = 0;
		for (auto& pCard : _cards)
		{
			auto request = assets.LoadAssetAsync(pCard->GetAssetID(), AsyncTask::LoadCoverImage, priority--);
			pCard->SetPendingCoverImage(std::move(request.future));
			pCard->EnableTags(_bEnableTags);

			if (initCounter++ < 8)
				pCard->Initialize();
			else
				pCard->Cull(true);
		}

		InvalidateLayout();
	}

	void CardList::OnUpdate(float fElapsed)
	{
		ScrollPanel::OnUpdate(fElapsed);

		int32_t curr_rows = toI(_pGridSizer->GetRows());

		if (_last_rows != curr_rows)
		{
			auto height = GetHeight();
			auto kCardHeight = cardHeight(_cardSize);
			auto last_extent = (_last_rows * kCardHeight + std::max(_last_rows - 1, 0) * Constants::GUI::CardSpacingY);
			auto curr_extent = (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::CardSpacingY);

			_maxExtent = curr_extent;
			_last_rows = curr_rows;

			if (last_extent > 0)
			{
				float ratio = _fScrollY / last_extent;
				_fScrollY = ratio * toF(_maxExtent);
				_fTargetScrollY = _fScrollY;
				LayoutNow();
			}
		}
	}

	void CardList::SetFilter(const fig::string& search_string) noexcept
	{
		if (fig::util::empty_or_whitespace(search_string))
		{
			ClearFilter();
			return;
		}

		SearchQuery query { search_string };

		for (auto& card : _cards)
		{
			bool bFiltered = card->IsFilteredBy(query);
//			card->SetVisible(not bFiltered);
//			card->EnableLayout(not bFiltered);
			card->SetHidden(bFiltered);
		}

		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Sorting, SortBy::CreatedAt);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Ordering, OrderBy::Descending);
		Sort(sortBy, orderBy);
		
		std::stable_partition(_cards.begin(), _cards.end(), [](auto& card) { return !card->IsHidden(); });

		// Reorder grid
		_pGridSizer->RemoveAll();
		for (auto& card : _cards)
			_pGridSizer->Add(card);

		InvalidateLayout();
	}

	void CardList::ClearFilter() noexcept
	{
		for (auto& card : _cards)
		{
//			card->SetVisible(true);
//			card->EnableLayout(true);
			card->SetHidden(false);
		}
		Reorder();
		InvalidateLayout();
	}

	void CardList::SetCardSize(CardSize cardSize)
	{
		if (cardSize == _cardSize)
			return;

		_cardSize = cardSize;

		for (auto& card : _cards)
			card->SetCardSize(cardSize);

		_pGridSizer->SetCellSize(cardWidth(cardSize), cardHeight(cardSize));
		_pGridSizer->SetSpacing(Constants::GUI::CardSpacingX, Constants::GUI::CardSpacingY);

		_fScrollY = 0;
		InvalidateLayout();
	}

	void CardList::EnableTags(bool bEnable) noexcept
	{
		if (_bEnableTags == bEnable)
			return;
		_bEnableTags = bEnable;

		for (auto& card : _cards)
			card->EnableTags(bEnable);
	}

	void CardList::Reset()
	{
		DestroyChildren();
		_cards.clear();
		_fScrollY = 0;
		_fTargetScrollY = 0;
	}

	void CardList::OnScroll()
	{
		MainFrame::GetInstance().PopAllMenus();
	}

	void CardList::OnAfterLayout()
	{
		int32_t curr_rows = toI(_pGridSizer->GetRows());
		Coord kCardHeight = cardHeight(_cardSize);
		_maxExtent = (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::CardSpacingY);

		ScrollPanel::OnAfterLayout();
	}

	void CardList::Sort(SortBy sortBy, OrderBy orderBy)
	{
		auto fnCompare = [](const fig::timestamp& a, const fig::timestamp& b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};

		// Initial sort (creation date)
		std::ranges::stable_sort(_cards, [&](CoverCard* a, CoverCard* b) -> bool {
			auto& meta_a = a->GetMetaData();
			auto& meta_b = b->GetMetaData();
			int cmp = fnCompare(meta_a.createdAt, meta_b.createdAt);
			if (orderBy == OrderBy::Descending)
				cmp *= -1;
			return cmp < 0;
		});

		// Then sort by...
		if (sortBy != SortBy::CreatedAt)
		{
			std::ranges::stable_sort(_cards, [&](CoverCard* a, CoverCard* b) -> bool {
				auto& meta_a = a->GetMetaData();
				auto& meta_b = b->GetMetaData();
				int cmp = 0;
				switch (sortBy)
				{
				case SortBy::Name:
					cmp = _stricmp(meta_a.name.c_str(), meta_b.name.c_str());
					break;
				case SortBy::UpdatedAt:
					cmp = fnCompare(meta_a.updatedAt, meta_b.updatedAt);
					break;
				case SortBy::LastMessaged:
					cmp = fnCompare(meta_a.lastUsedAt, meta_b.lastUsedAt);
					break;
				case SortBy::ChatCount:
					cmp = 0; //!
					break;
				}

				if (orderBy == OrderBy::Descending)
					cmp *= -1;
				return cmp < 0;
			});
		}
	}

	void CardList::Reorder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Sorting, SortBy::CreatedAt);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Ordering, OrderBy::Descending);
		Sort(sortBy, orderBy);

		std::stable_partition(_cards.begin(), _cards.end(), [](auto& card) { return !card->IsHidden(); });

		// Reorder grid
		_pGridSizer->RemoveAll();
		for (auto& card : _cards)
			_pGridSizer->Add(card);

		InvalidateLayout();
	}
}