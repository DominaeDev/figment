#include <pch.h>
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/AssetManager.h"
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
		auto& assetMngr = Global::GetUserManager().GetContent().GetAssetManager();
		const auto& profileId = Global::GetUserManager().GetActiveProfile().id;

		Reset();

		if (cardType == CardType::Scenario)
		{
			// Find scenarios
			auto scenarios = assetMngr.GetScenarioAssets() | std::ranges::to<std::vector>();
			std::sort(scenarios.begin(), scenarios.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : scenarios)
			{
				DEBUG_MEASURE_BEGIN(std::format("Scenario card {}:", asset.id.to_str()));
				auto pCard = new ScenarioCard(this, asset.id, _cardSize);
				pCard->SetDelegate([this](auto& card) { Reorder(); });
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
				DEBUG_MEASURE_END();
			}
		}

		// Find characters
		if (cardType == CardType::Character)
		{
			auto characters = assetMngr.GetCharacterAssets() | std::ranges::to<std::vector>();
			std::sort(characters.begin(), characters.end(), [](const Asset& a, const Asset& b) {return a.GetCreatedAt() < b.GetCreatedAt(); });

			for (auto& asset : characters)
			{
				DEBUG_MEASURE_BEGIN(std::format("Character card {}:", asset.id.to_str()));
				auto pCard = new CharacterCard(this, asset.id, _cardSize);
				pCard->SetDelegate([this](auto& card) { Reorder(); });
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);
				DEBUG_MEASURE_END();
			}
		}

		Reorder();

		size_t initCounter = 0;
		int32_t priority = 0;
		for (auto& pCard : _cards)
		{
			auto request = assetMngr.LoadAssetAsync(pCard->GetAssetID(), AsyncTask::LoadCoverImage, priority--);
			pCard->SetPendingCoverImage(std::move(request.future));
			pCard->ShowTags(_bEnableTags);

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
		_filterString = search_string;
		
		Reorder();
		ResetScroll();
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
			card->ShowTags(bEnable);
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

	static void Sort(std::vector<CoverCard*>& cards, SortBy sortBy, OrderBy orderBy)
	{
		auto fnCompare = [](const fig::timestamp& a, const fig::timestamp& b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};
		auto fnCompareCount = [](uint32_t a, uint32_t b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};

		// Initial sort (creation date)
		std::ranges::stable_sort(cards, [&](CoverCard* a, CoverCard* b) -> bool {
			auto& meta_a = a->GetMetaData();
			auto& meta_b = b->GetMetaData();
			int cmp = fnCompare(meta_b.lastUsedAt, meta_a.lastUsedAt);
			return cmp < 0;
		});

		// Then sort by...
		if (sortBy != SortBy::LastUsedAt)
		{
			std::ranges::stable_sort(cards, [&](CoverCard* a, CoverCard* b) -> bool {
				auto& meta_a = a->GetMetaData();
				auto& meta_b = b->GetMetaData();
				int cmp = 0;
				switch (sortBy)
				{
				case SortBy::Name:
					cmp = _stricmp(meta_a.name.c_str(), meta_b.name.c_str());
					break;
				case SortBy::CreatedAt:
					cmp = fnCompare(meta_a.createdAt, meta_b.createdAt);
					break;
				case SortBy::UpdatedAt:
					cmp = fnCompare(meta_a.updatedAt, meta_b.updatedAt);
					break;
				case SortBy::ChatCount:
					cmp = fnCompareCount(meta_a.chatCount, meta_b.chatCount);
					break;
				}
				if (orderBy == OrderBy::Descending)
					cmp *= -1;
				return cmp < 0;
			});
		}
		else if (orderBy == OrderBy::Ascending)
		{
			std::ranges::reverse(cards);
		}
	}

	static void Filter(std::vector<CoverCard*>& cards, FilterFlags filterBy, const fig::string& search_string)
	{
		SearchQuery query { search_string };

		auto fnFilter = [&](const CoverCard* card) {
			return card->MatchesFlags(filterBy) and card->MatchesSearch(query);
		};

		for (auto& card : cards)
			card->SetHidden(not fnFilter(card));
	}

	void CardList::Reorder()
	{
		// Sort
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Ordering, OrderBy::Default);
		Sort(_cards, sortBy, orderBy);

		// Filter
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Filtering, DefaultFilterFlags, FilterFlagMapping);
		Filter(_cards, filterBy, _filterString);

		// Move visible cards to front
		std::stable_partition(_cards.begin(), _cards.end(), [](auto& card) { return !card->IsHidden(); });

		// Update grid
		_pGridSizer->RemoveAll();
		for (auto& card : _cards)
		{
			_pGridSizer->Add(card);
			card->ResetHoverZoom();
		}

		InvalidateLayout();
	}

	void CardList::ResetScroll() noexcept
	{
		ScrollTo(0, false);
		InvalidateLayout();
	}
}