#include "gui/CardList.h"
#include "gui/GridSizer.h"
#include "io/AssetManager.h"
#include "io/ContentTypes.h"
#include "user/UserManager.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	constexpr fig::coord TopMargin = 8;
	constexpr fig::coord BottomMargin = 120;

	static constexpr fig::coord cardWidth(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Full ? Constants::GUI::CardWidth : Constants::GUI::HalfCardWidth;
	}

	static constexpr fig::coord cardHeight(CardSize cardSize) noexcept
	{
		return cardSize == CardSize::Full ? Constants::GUI::CardHeight : Constants::GUI::HalfCardHeight;
	}

	template <typename T>
	CardList<T>::CardList(ControlPtr pParent, CardSize cardSize) : ScrollPanel(pParent)
	{
		_pGridSizer = SetSizer<GridSizer>(cardWidth(cardSize), cardHeight(cardSize));
		_pGridSizer->SetSpacing(Constants::GUI::Cards::SpacingX, Constants::GUI::Cards::SpacingY);
		_pGridSizer->EnableCentering(true);
		SetTopPadding(TopMargin);
		SetBottomPadding(BottomMargin);

		EnableClipping(true);
		EnableCulling(true);
	}

	template <typename T>
	void CardList<T>::OnUpdate(float fElapsed)
	{
		ScrollPanel::OnUpdate(fElapsed);

		int32_t curr_rows = toI(_pGridSizer->GetRows());

		if (_last_rows != curr_rows)
		{
			auto height = GetHeight();
			auto kCardHeight = cardHeight(_cardSize);
			auto last_extent = (_last_rows * kCardHeight + std::max(_last_rows - 1, 0) * Constants::GUI::Cards::SpacingY);
			auto curr_extent = (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::Cards::SpacingY);

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

	template <typename T>
	void CardList<T>::SetFilter(const fig::string& search_string) noexcept
	{
		_filterString = search_string;

		Reorder();
		ResetScroll();
	}

	template <typename T>
	void CardList<T>::SetCardSize(CardSize cardSize)
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

	template <typename T>
	void CardList<T>::EnableTags(bool bEnable) noexcept
	{
		if (_bEnableTags == bEnable)
			return;
		_bEnableTags = bEnable;

		for (auto& card : _cards)
			card->ShowTags(bEnable);
	}

	template <typename T>
	void CardList<T>::Clear()
	{
		DestroyChildren();
		_cards.clear();
		_fScrollY = 0;
		_fTargetScrollY = 0;
		_bInitialized = false;
	}

	template <typename T>
	void CardList<T>::OnScroll()
	{
		PushEvent(UserEvent::Scrolling);
	}

	template <typename T>
	fig::coord CardList<T>::GetExtent() const
	{
		int32_t curr_rows = toI(_pGridSizer->GetRows());
		fig::coord kCardHeight = cardHeight(_cardSize);
		return (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::Cards::SpacingY);
	}

	template <typename T, typename TCard = CardTypeOf<T>::Type, typename CardPtr = fig::observer_ptr<TCard>>
	requires std::derived_from<TCard, CoverCard>
	static void Sort(std::vector<CardPtr>& cards, SortBy sortBy, OrderBy orderBy)
	{
		auto fnCompare = [](const fig::timestamp& a, const fig::timestamp& b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};
		auto fnCompareCount = [](size_t a, size_t b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};

		// Initial sort (creation date)
		std::ranges::stable_sort(cards, [&](CardPtr a, CardPtr b) -> bool {
			auto& meta_a = a->GetMetaData();
			auto& meta_b = b->GetMetaData();
			int cmp = fnCompare(meta_b.lastUsedAt, meta_a.lastUsedAt);
			return cmp < 0;
		});

		// Then sort by...
		std::ranges::stable_sort(cards, [&](CardPtr a, CardPtr b) -> bool {
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

	template <typename T, typename TCard = CardTypeOf<T>::Type, typename CardPtr = fig::observer_ptr<TCard>>
	requires std::derived_from<TCard, CoverCard>
	static void Filter(std::vector<CardPtr>& cards, FilterFlags filterBy, const fig::string& search_string)
	{
		SearchQuery query { search_string };

		auto fnFilter = [&](auto&& card) {
			return card->MatchesFlags(filterBy) and card->MatchesSearch(query);
		};

		for (auto& card : cards)
			card->SetHidden(not fnFilter(card));
	}

	template <typename T>
	void CardList<T>::Reorder()
	{
		auto [sortBy, orderBy, filterBy] = GetSortOrder();

		// Filter
		Filter<T>(_cards, filterBy, _filterString);

		// Sort
		Sort<T>(_cards, sortBy, orderBy);

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

	template <typename T>
	void CardList<T>::OnDeleteCard(CardPtr pCard)
	{
		if (auto try_card = std::ranges::find_if(_cards, [pCard](auto& c) {
			return c.get() == pCard;
			}); try_card != std::ranges::end(_cards))
		{
			if (Global::GetUserContent().DeleteAsset((*try_card)->GetAssetId()))
				RefreshCards();
		}
	}

	template <typename T>
	void CardList<T>::OnCardEvent(CardPtr pCard, CardEvent event)
	{
		switch (event)
		{
		case CardEvent::Refresh:
			Reorder();
			break;
		case CardEvent::Delete:
			OnDeleteCard(pCard);
			break;
		}
	}

	template <typename T>
	void CardList<T>::CreateCards()
	{
		if (_bInitialized)
			return;

		Clear();

		// Find assets
		auto assets = Global::GetUserContent().GetAssets().FindAssetsOfType(AssetTypeOf<T>)
			| std::views::filter([](auto&& s) { return s.get().parent_id.empty(); })
			| std::ranges::to<std::vector>();

		std::sort(assets.begin(), assets.end(), [](const Asset& a, const Asset& b) { return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset_ref : assets)
		{
			auto& asset = asset_ref.get();
			DEBUG_MEASURE_BEGIN(std::format("Create card {}", asset.id.to_str()));
			auto pCard = CreateControl<TCard>(asset.id, _cardSize);
			pCard->SetDelegate([this](CoverCard& card, CardEvent event) { OnCardEvent(static_cast<TCard*>(&card), event); });
			pCard->ShowTags(_bEnableTags);
			_pGridSizer->Add(pCard);
			_cards.push_back(pCard);
			DEBUG_MEASURE_END();
		}

		Reorder();

		size_t initCounter = 0;
		int32_t priority = 0;
		for (auto& pCard : _cards)
		{
			auto request = Global::GetUserContent().GetAssets().LoadAssetAsync(pCard->GetAssetId(), AsyncTask::LoadCoverImage, priority--);
			pCard->SetPendingCoverImage(std::move(request.future));

			if (initCounter++ < 8)
				pCard->Initialize(); // Load first 8 cards synchronously
			else
				pCard->Cull(true);
		}

		_bInitialized = true;
		InvalidateLayout();
	}

	template <typename T>
	void CardList<T>::RefreshCards()
	{
		if (not _bInitialized)
			return;

		auto assets = Global::GetUserContent().GetAssets().FindAssetsOfType(AssetTypeOf<T>)
			| std::views::filter([](auto&& s) { return s.get().parent_id.empty(); })
			| std::ranges::to<std::vector>();

		std::map<fig::uuid, fig::timestamp> updateTimes;
		for (auto& asset : assets)
			updateTimes[asset.get().id] = asset.get().GetUpdatedAt();

		std::unordered_set<fig::uuid> existingAssetIds;

		for (size_t i = 0uz; i < _cards.size();)
		{
			auto& pCard = _cards[i];
			existingAssetIds.insert(pCard->GetAssetId());

			if (auto it = updateTimes.find(pCard->GetAssetId()); it != updateTimes.cend())
			{
				auto& updatedAt = it->second;
				if (pCard->GetUpdatedAt() < updatedAt) // Updated
				{
					auto request = Global::GetUserContent().GetAssets().LoadAssetAsync(pCard->GetAssetId(), AsyncTask::LoadCoverImage, 0);
					pCard->SetPendingCoverImage(std::move(request.future));
					pCard->RefreshFull();
				}
				else
					pCard->RefreshMeta();
			}
			else
			{
				// Card removed
				DestroyChild(pCard);
				_cards[i] = nullptr;
			}
			++i;
		}
		std::erase_if(_cards, [](auto&& p) { return !p; });

		// Create new cards
		for (auto& asset : assets)
		{
			auto& assetId = asset.get().id;

			if (not existingAssetIds.contains(assetId))
			{
				auto pCard = CreateControl<TCard>(assetId, _cardSize);
				pCard->SetDelegate([this](CoverCard& card, CardEvent event) { OnCardEvent(static_cast<TCard*>(&card), event); });
				pCard->ShowTags(_bEnableTags);
				_pGridSizer->Add(pCard);
				_cards.push_back(pCard);

				auto request = Global::GetUserContent().GetAssets().LoadAssetAsync(pCard->GetAssetId(), AsyncTask::LoadCoverImage, 0);
				pCard->SetPendingCoverImage(std::move(request.future));
			}
		}

		Reorder();
	}
}