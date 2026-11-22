#include <pch.h>
#include "gui/UserPersonaList.h"
#include "gui/GridSizer.h"
#include "gui/Menu.h"
#include "io/AssetManager.h"
#include "io/ContentManager.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	constexpr fig::coord TopMargin = 8;
	constexpr fig::coord BottomMargin = 120;

	UserPersonaList::UserPersonaList(ControlPtr pParent) : ScrollPanel(pParent)
	{
		_pGridSizer = SetSizer<GridSizer>(UserPersonaListItem::Width, UserPersonaListItem::Height);
		_pGridSizer->SetSpacing(Constants::GUI::Cards::SpacingX, Constants::GUI::Cards::SpacingY);
		_pGridSizer->EnableCentering(true);
		SetTopPadding(TopMargin);
		SetBottomPadding(BottomMargin);

		EnableClipping(true);
		EnableCulling(true);
	}

	void UserPersonaList::OnUpdate(float fElapsed)
	{
		ScrollPanel::OnUpdate(fElapsed);

		int32_t curr_rows = toI(_pGridSizer->GetRows());

		if (_last_rows != curr_rows)
		{
			auto height = GetHeight();
			auto kCardHeight = UserPersonaListItem::Height;
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

	void UserPersonaList::SetFilter(const fig::string& search_string) noexcept
	{
		_filterString = search_string;

		Reorder();
		ResetScroll();
	}

	void UserPersonaList::Clear()
	{
		DestroyChildren();
		_items.clear();
		_fScrollY = 0;
		_fTargetScrollY = 0;
		_bInitialized = false;
	}

	void UserPersonaList::OnScroll()
	{
		PushEvent(UserEvent::Scrolling);
	}

	fig::coord UserPersonaList::GetExtent() const
	{
		int32_t curr_rows = toI(_pGridSizer->GetRows());
		fig::coord kCardHeight = UserPersonaListItem::Height;
		return (curr_rows * kCardHeight + std::max(curr_rows - 1, 0) * Constants::GUI::Cards::SpacingY);
	}

	static void Sort(std::vector<fig::observer_ptr<UserPersonaListItem>>& cards, SortBy sortBy, OrderBy orderBy)
	{
		auto fnCompare = [](const fig::timestamp& a, const fig::timestamp& b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};
		auto fnCompareCount = [](size_t a, size_t b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};

		// Initial sort (creation date)
		std::ranges::stable_sort(cards, [&](fig::observer_ptr<UserPersonaListItem> a, fig::observer_ptr<UserPersonaListItem> b) -> bool {
			auto& meta_a = a->GetMetaData();
			auto& meta_b = b->GetMetaData();
			int cmp = fnCompare(meta_b.lastUsedAt, meta_a.lastUsedAt);
			return cmp < 0;
		});

		// Then sort by...
		std::ranges::stable_sort(cards, [&](fig::observer_ptr<UserPersonaListItem> a, fig::observer_ptr<UserPersonaListItem> b) -> bool {
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
			case SortBy::LastUsedAt:
				cmp = fnCompare(meta_a.lastUsedAt, meta_b.lastUsedAt);
				break;
			}
			if (orderBy == OrderBy::Descending)
				cmp *= -1;
			return cmp < 0;
		});
	}

	static void Filter(std::vector<fig::observer_ptr<UserPersonaListItem>>& cards, FilterFlags filterBy, const fig::string& search_string)
	{
		SearchQuery query { search_string };

		auto fnFilter = [&](auto&& card) {
			return card->MatchesFlags(filterBy) and card->MatchesSearch(query);
		};

		for (auto& card : cards)
			card->SetHidden(not fnFilter(card));
	}

	void UserPersonaList::Reorder()
	{
		auto [sortBy, orderBy, filterBy] = GetSortOrder();

		// Filter
		Filter(_items, filterBy, _filterString);

		// Sort
		Sort(_items, sortBy, orderBy);

		// Move visible cards to front
		std::stable_partition(_items.begin(), _items.end(), [](auto& card) { return !card->IsHidden(); });

		// Update grid
		_pGridSizer->RemoveAll();
		for (auto& card : _items)
			_pGridSizer->Add(card);

		InvalidateLayout();
	}

	void UserPersonaList::CreateCards()
	{
		Clear();

		// Find assets
		auto assets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::UserPersona))
			| std::views::filter([](auto&& asset) { return asset.get().parent_id.empty(); })
			| std::ranges::to<std::vector>();

		std::sort(assets.begin(), assets.end(), [](const Asset& a, const Asset& b) { return a.GetCreatedAt() < b.GetCreatedAt(); });

		for (auto& asset_ref : assets)
		{
			auto& asset = asset_ref.get();
			auto pItem = CreateControl<UserPersonaListItem>(asset.id);
			pItem->SetDelegate([this](UserPersonaListItem& item, UserPersonaListItemEvent event) { OnItemEvent(item, event); });
			_pGridSizer->Add(pItem);
			_items.push_back(pItem);
		}

		Reorder();

		_bInitialized = true;
		InvalidateLayout();
	}

	void UserPersonaList::RefreshCards()
	{
		if (not _bInitialized)
			return;

		auto assets = Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::UserPersona))
			| std::views::filter([](auto&& a) { return a.get().parent_id.empty(); })
			| std::ranges::to<std::vector>();

		std::map<fig::uuid, fig::timestamp> updateTimes;
		for (auto& asset : assets)
			updateTimes[asset.get().id] = asset.get().GetUpdatedAt();

		std::unordered_set<fig::uuid> existingAssetIds;

		for (size_t i = 0uz; i < _items.size();)
		{
			auto& pCard = _items[i];
			existingAssetIds.insert(pCard->GetAssetId());

			if (auto it = updateTimes.find(pCard->GetAssetId()); it != updateTimes.cend())
			{
				auto& updatedAt = it->second;
				if (pCard->GetUpdatedAt() < updatedAt) // Updated
					pCard->RefreshFull();
			}
			else
			{
				// Card removed
				DestroyChild(pCard);
				_items[i] = nullptr;
			}
			++i;
		}
		std::erase_if(_items, [](auto&& p) { return !p; });

		// Create new cards
		for (auto& asset : assets)
		{
			auto& assetId = asset.get().id;

			if (not existingAssetIds.contains(assetId))
			{
				auto pItem = CreateControl<UserPersonaListItem>(assetId);
				pItem->SetDelegate([this](UserPersonaListItem& item, UserPersonaListItemEvent event) { OnItemEvent(item, event); });
				_pGridSizer->Add(pItem);
				_items.push_back(pItem);
			}
		}

		Reorder();
	}

	std::tuple<SortBy, OrderBy, FilterFlags> UserPersonaList::GetSortOrder()
	{
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::Interface::UserPersonaList::Sorting, SortBy::Default);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::Interface::UserPersonaList::Ordering, OrderBy::Default);
		auto filterBy = Global::GetUserSettings().GetFlags<FilterFlags>(UserSetting::Interface::UserPersonaList::Filtering, DefaultUserPersonaFilterFlags, FilterFlagMapping);
		return std::make_tuple(sortBy, orderBy, filterBy);
	}

	void UserPersonaList::OnItemEvent(UserPersonaListItem& item, UserPersonaListItemEvent event)
	{
		switch (event)
		{
		case UserPersonaListItemEvent::Refresh:
			for (auto& item : _items)
				item->RefreshFull();
			break;
		case UserPersonaListItemEvent::Hidden:
			Reorder();
			break;
		case UserPersonaListItemEvent::Delete:
			DeleteUserPersona(item);
			break;
		}
	}

	void UserPersonaList::DeleteUserPersona(UserPersonaListItem& item)
	{
		if (auto try_item = std::ranges::find_if(_items, [&item](auto& i) { 
			return i.get() == &item;
		}); try_item != std::ranges::end(_items))
		{
			if (Global::GetUserContent().DeleteAsset((*try_item)->GetAssetId()))
			{
				DestroyChild(*try_item);
				_items.erase(try_item);
				Reorder();
			}
		}
	}

}