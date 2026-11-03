#include <pch.h>
#include "gui/StoryCardList.h"
#include "app/AppState.h"
#include "user/UserManager.h"
#include "io/AssetManager.h"
#include "gui/GridSizer.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	constexpr fig::coord TopMargin = 8;
	constexpr fig::coord BottomMargin = 120;

	using ScenarioCardPtr = fig::observer_ptr<ScenarioCard>;

	static constexpr fig::coord cardWidth(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Full ? Constants::GUI::CardWidth : Constants::GUI::HalfCardWidth;
	}

	static constexpr fig::coord cardHeight(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Full ? Constants::GUI::CardHeight : Constants::GUI::HalfCardHeight;
	}

	StoryCardList::StoryCardList(ControlPtr pParent, CardSize cardSize) : ScrollPanel(pParent),
		_cardSize { cardSize }
	{
		_pGridSizer = SetSizer<GridSizer>(cardWidth(cardSize), cardHeight(cardSize));
		_pGridSizer->SetSpacing(Constants::GUI::Cards::SpacingX, Constants::GUI::Cards::SpacingY);
		_pGridSizer->EnableCentering(true);
		SetTopPadding(TopMargin);
		SetBottomPadding(BottomMargin);

		EnableClipping(true);
		EnableCulling(true);
	}

	void StoryCardList::CreateCards()
	{
		if (_bInitialized)
			return;

		// Create cards
		auto& userContent = Global::GetUserContent();

		Clear();

		// Find scenarios

		auto scenarios = userContent.GetScenarios();
		std::sort(scenarios.begin(), scenarios.end(), [](const Asset& a, const Asset& b) { return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset_ref : scenarios)
		{
			auto& asset = asset_ref.get();

			auto pCard = CreateControl<ScenarioCard>(asset.id, _cardSize);
			pCard->SetDelegate([this](CoverCard& card, CardEvent event) { OnCardEvent(card, event); });
			pCard->ShowTags(_bEnableTags);
			_pGridSizer->Add(pCard);
			_cards.push_back(pCard);

		}

		Reorder();

		size_t initCounter = 0;
		int32_t priority = 0;
		for (auto& pCard : _cards)
		{
			auto request = userContent.GetAssets().LoadAssetAsync(pCard->GetAssetID(), AsyncTask::LoadCoverImage, priority--);
			pCard->SetPendingCoverImage(std::move(request.future));

			if (initCounter++ < 8)
				pCard->Initialize(); // Load first 8 cards synchronously
			else
				pCard->Cull(true);
		}

		_bInitialized = true;
		InvalidateLayout();
	}

	void StoryCardList::OnUpdate(float fElapsed)
	{
		ScrollPanel::OnUpdate(fElapsed);

		int32_t curr_rows = toI(_pGridSizer->GetRows());

		if (_last_rows != curr_rows)
		{
			auto height = GetHeight();
			auto kCardHeight = cardHeight(_cardSize);
			auto last_extent = (_last_rows * kCardHeight + std::max(_last_rows - 1, 0) * Constants::GUI::Cards::SpacingY);
			auto curr_extent = (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::Cards::SpacingY);

//			_maxExtent = curr_extent;
			_last_rows = curr_rows;

			if (last_extent > 0)
			{
				float ratio = _fScrollY / last_extent;
				_fScrollY = ratio * toF(curr_extent);
				_fTargetScrollY = _fScrollY;
				LayoutNow();
			}
		}
	}

	void StoryCardList::SetFilter(const fig::string& search_string) noexcept
	{
		_filterString = search_string;

		Reorder();
		ResetScroll();
	}

	void StoryCardList::SetCardSize(CardSize cardSize)
	{
		if (cardSize == _cardSize)
			return;

		_cardSize = cardSize;

		for (auto& card : _cards)
			card->SetCardSize(cardSize);

		_pGridSizer->SetCellSize(cardWidth(cardSize), cardHeight(cardSize));
		_pGridSizer->SetSpacing(Constants::GUI::Cards::SpacingX, Constants::GUI::Cards::SpacingY);

		_fScrollY = 0;
		InvalidateLayout();
	}

	void StoryCardList::EnableTags(bool bEnable) noexcept
	{
		if (_bEnableTags == bEnable)
			return;
		_bEnableTags = bEnable;

		for (auto& card : _cards)
			card->ShowTags(bEnable);
	}

	void StoryCardList::Clear()
	{
		DestroyChildren();
		_cards.clear();
		_fScrollY = 0;
		_fTargetScrollY = 0;
		_bInitialized = false;
	}

	void StoryCardList::OnScroll()
	{
		PushEvent(UserEvent::Scrolling);
	}

	fig::coord StoryCardList::GetExtent() const
	{
		int32_t curr_rows = toI(_pGridSizer->GetRows());
		fig::coord kCardHeight = cardHeight(_cardSize);
		return (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::Cards::SpacingY);
	}

	static void Sort(std::vector<ScenarioCardPtr>& cards, SortBy sortBy, OrderBy orderBy)
	{
		auto fnCompare = [](const fig::timestamp& a, const fig::timestamp& b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
			};
		auto fnCompareCount = [](size_t a, size_t b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
			};

			// Initial sort (creation date)
		std::ranges::stable_sort(cards, [&](ScenarioCardPtr a, ScenarioCardPtr b) -> bool {
			auto& meta_a = a->GetMetaData();
			auto& meta_b = b->GetMetaData();
			int cmp = fnCompare(meta_b.lastUsedAt, meta_a.lastUsedAt);
			return cmp < 0;
		});

		// Then sort by...
		std::ranges::stable_sort(cards, [&](ScenarioCardPtr a, ScenarioCardPtr b) -> bool {
			auto& meta_a = a->GetMetaData();
			auto& meta_b = b->GetMetaData();
			size_t chats_a = a->GetChatCount();
			size_t chats_b = b->GetChatCount();
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
			case SortBy::LastUsedAt:
				cmp = fnCompare(meta_a.lastUsedAt, meta_b.lastUsedAt);
				break;
			case SortBy::ChatCount:
				cmp = fnCompareCount(chats_a, chats_b);
				break;
			}
			if (orderBy == OrderBy::Descending)
				cmp *= -1;
			return cmp < 0;
		});
	}

	static void Filter(std::vector<ScenarioCardPtr>& cards, FilterFlags filterBy, const fig::string& search_string)
	{
		SearchQuery query { search_string };

		auto fnFilter = [&](auto&& card) {
			return card->MatchesFlags(filterBy) and card->MatchesSearch(query);
			};

		for (auto& card : cards)
			card->SetHidden(not fnFilter(card));
	}

	void StoryCardList::Reorder()
	{
		// Filter
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::ScenarioList::Filtering, DefaultFilterFlags, FilterFlagMapping);
		Filter(_cards, filterBy, _filterString);

		// Sort
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::ScenarioList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::ScenarioList::Ordering, OrderBy::Default);
		Sort(_cards, sortBy, orderBy);

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

	void StoryCardList::DeleteScenario(CoverCard& card)
	{
		if (auto try_card = std::ranges::find_if(_cards, [&card](auto& c) {
			return c.get() == &card;
			}); try_card != std::ranges::end(_cards))
		{
			if (Global::GetUserContent().DeleteAsset((*try_card)->GetAssetID()))
				RefreshCards();
		}
	}

	void StoryCardList::OnCardEvent(CoverCard& card, CardEvent event)
	{
		switch (event)
		{
		case CardEvent::Refresh:
			Reorder();
			break;
		case CardEvent::Delete:
			DeleteScenario(card);
			break;
		}
	}

	void StoryCardList::RefreshCards()
	{
		if (not _bInitialized)
			return;

		auto& assets = Global::GetUserContent().GetAssets();

		auto scenarios = Global::GetUserContent().GetScenarios();
		std::map<fig::uuid, fig::timestamp> updateTimes;
		for (auto& scenario : scenarios)
			updateTimes[scenario.get().id] = scenario.get().GetUpdatedAt();

		std::unordered_set<fig::uuid> existingScenarioIds;

		for (size_t i = 0uz; i < _cards.size();)
		{
			auto& pCard = _cards[i];
			existingScenarioIds.insert(pCard->GetAssetID());

			if (auto it = updateTimes.find(pCard->GetScenarioId()); it != updateTimes.cend())
			{
				auto& updatedAt = it->second;
				if (pCard->GetUpdatedAt() < updatedAt) // Updated
				{
					auto request = assets.LoadAssetAsync(pCard->GetAssetID(), AsyncTask::LoadCoverImage, 0);
					pCard->SetPendingCoverImage(std::move(request.future));
					pCard->RefreshFull();
				}
				else
					pCard->RefreshMeta();
			}
			else
			{
				// Scenario removed
				DestroyChild(pCard);
				_cards[i] = nullptr;
			}
			++i;
		}
		std::erase_if(_cards, [](auto&& p) { return !p; });

		// Create new cards
		for (auto& scenario : scenarios)
		{
			auto& assetId = scenario.get().id;

			if (not existingScenarioIds.contains(assetId))
			{
				auto pCard = CreateControl<ScenarioCard>(assetId, _cardSize);
				pCard->SetDelegate([this](CoverCard& card, CardEvent event) { OnCardEvent(card, event); });
				pCard->ShowTags(_bEnableTags);
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);

				auto request = assets.LoadAssetAsync(pCard->GetAssetID(), AsyncTask::LoadCoverImage, 0);
				pCard->SetPendingCoverImage(std::move(request.future));
			}
		}

		Reorder();
	}
}