#include <pch.h>
#include "gui/ChatList.h"
#include "gui/Area.h"

#include "app/AppState.h"
#include "gui/VerticalSizer.h"
#include "gui/ChatListItem.h"

using namespace fig::io;
using namespace fig::user;

namespace fig::gui
{
	static constexpr Coord TopMargin = 8;
	static constexpr Coord Spacing = 8;
	static constexpr Coord BottomMargin = 120;

	static constexpr std::array<fig::string_view, 6> TimeBucketLabels {
		"Just now",
		"Earlier today",
		"Yesterday",
		"Past week",
		"Past month",
		"Older than a month",
	};

	ChatList::ChatList(ParentPtr pParent) : ScrollPanel(pParent)
	{
		_pVerticalSizer = SetSizer<VerticalSizer>();
		SetTopMargin(TopMargin);
		SetBottomMargin(BottomMargin);

		EnableClipping(true);
		EnableCulling(true);
	}

	ChatList::TimeBucket ChatList::GetTimeBucket(fig::timestamp then, fig::timestamp now) noexcept
	{
		auto minDiff = (now.to_local() - then.to_local()).minutes();
		auto monthDiff = (now.to_local() - then.to_local()).months();

		auto thenDay = std::chrono::floor<std::chrono::days>(static_cast<std::chrono::local_time<std::chrono::milliseconds>>(then));
		auto nowDay = std::chrono::floor<std::chrono::days>(static_cast<std::chrono::local_time<std::chrono::milliseconds>>(now));
		auto dayDiff = nowDay - thenDay;

		if (minDiff < 10)
			return TimeBucket::LessThan5Minutes;
		else if (dayDiff < std::chrono::days(1))
			return TimeBucket::LessThan1Day;
		else if (dayDiff < std::chrono::days(2))
			return TimeBucket::LessThan2Days;
		else if (dayDiff < std::chrono::days(7))
			return TimeBucket::LessThan1Week;
		else if (monthDiff < 1)
			return TimeBucket::LessThan1Month;
		return TimeBucket::Older;
	}

	void ChatList::ShowAllChats()
	{
		auto chats = Global::GetUserContent().GetChatLogs(true)
			| std::ranges::to<std::vector>();
		ShowChats(chats);
	}

	void ChatList::ShowChatsWith(const fig::uuid& characterId)
	{
		auto chats = Global::GetUserContent().GetChatLogsWith(characterId, true)
			| std::ranges::to<std::vector>();
		ShowChats(chats);
	}

	void ChatList::ShowChats(const fig::cref_vector<Asset>& chats)
	{
		Reset();

		auto& content = Global::GetUserContent();

		auto now = fig::now();
		_items = chats
			| std::views::transform([&content, now](auto& a) {
				auto& asset = a.get();
				auto chat = content.Get<fig::data::ChatLog>(asset.id);
				return Item {
					.assetId = asset.id,
					.chatLog = chat,
					.createdAt = asset.GetCreatedAt(),
					.updatedAt = asset.GetUpdatedAt(),
					.timeBucket = GetTimeBucket(asset.GetUpdatedAt(), now),
				};
			})
			| std::ranges::to<std::vector>();

		Reorder();
	}

	ControlPtr ChatList::CreateHeader(fig::string_view text)
	{
		auto panel = CreateControl<Area>();
		panel->SetMaxSize(Constants::GUI::ChatList::Width, -1);
		panel->SetHeight(40);

		auto label = panel->CreateControl<StaticText>("", FontFace::Italic, 18.0);
		label->SetTextAndResize(text);
		label->CenterVertically();
		return panel;
	}

	void ChatList::Reset()
	{
		DestroyChildren();
		ResetScroll();
		_items.clear();
	}

	void ChatList::OnScroll()
	{
		PushEvent(UserEvent::Scrolling);
	}

	void ChatList::OnAfterLayout()
	{
		if (_children.empty())
			_maxExtent = 0;
		else
		{
			auto& bottomItem = _children.back();
			_maxExtent = bottomItem->GetY() + bottomItem->GetHeight();
		}

		ScrollPanel::OnAfterLayout();
	}

	void ChatList::SetFilter(const fig::string& search_string) noexcept
	{
		_filterString = search_string;

		Reorder();
		ResetScroll();
	}

	void ChatList::Sort(SortBy sortBy, OrderBy orderBy)
	{
		auto fnCompare = [](const fig::timestamp& a, const fig::timestamp& b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};
		auto fnCompareCount = [](uint32_t a, uint32_t b) -> int {
			return a < b ? -1 : (a > b ? 1 : 0);
		};

		std::ranges::stable_sort(_items, [&](const Item& a, const Item& b) -> bool {
			int cmp = 0;
			switch (sortBy)
			{
				case SortBy::CreatedAt:
					cmp = fnCompare(a.createdAt, b.createdAt);
					break;
				case SortBy::UpdatedAt:
					cmp = fnCompare(a.updatedAt, b.updatedAt);
					break;
			}
			if (orderBy == OrderBy::Descending)
				cmp *= -1;
			return cmp < 0;
		});
	}

	bool ChatList::Item::MatchesFlags(ChatFilterFlags filter) noexcept
	{
		auto userSettings = Global::GetUserContent().GetUserSettings(assetId);

		if (filter.IsSet(ChatFilterFlag::Hidden) != userSettings.HasFlag(ContentUserSettings::Flag::Hidden))
			return false;

		if (filter.IsSet(ChatFilterFlag::Starred))
			return userSettings.HasFlag(ContentUserSettings::Flag::Favorite);

		return true;
	}

	void ChatList::Filter(ChatFilterFlags filterBy, const fig::string& search_string)
	{
		SearchQuery query { search_string };

		for (auto& item : _items)
		{
			item.filtered = !item.chatLog 
				or not item.MatchesFlags(filterBy)
				or !(*item.chatLog).GetSearchIndex().Match(query);
		}
	}

	void ChatList::Reorder()
	{
		// Filter
		auto filterBy = Global::GetUserSettings().GetFlags<ChatFilterFlags>(UserSetting::ChatList_Filtering, DefaultChatFilterFlags, ChatFilterFlagMapping);
		Filter(filterBy, _filterString);

		// Sort
		auto sortBy = Global::GetUserSettings().GetEnum<SortBy>(UserSetting::ChatList_Sorting, SortBy::LastUsedAt);
		auto orderBy = Global::GetUserSettings().GetEnum<OrderBy>(UserSetting::ChatList_Ordering, OrderBy::Default);
		Sort(sortBy, orderBy);

		DestroyChildren();

		auto chatsByTime = _items
			| std::views::filter([](auto& it) { return not it.filtered; })
			| std::ranges::to<std::vector>()
			| fig::group_by([](auto& it) { return it.timeBucket; });

		Clock user_clock_setting = Global::GetUserSettings().GetEnum<Clock>(UserSetting::Clock, ClockMapping);

		for (auto& kvp : chatsByTime)
		{
			auto bucket = kvp.first;
			auto& items = kvp.second;

			if (not _children.empty())
				_pVerticalSizer->AddSpacer(Spacing);

			// Header
			auto pHeader = CreateHeader(TimeBucketLabels[static_cast<size_t>(bucket)]);
			_pVerticalSizer->Add(pHeader, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);

			// Chats
			for (size_t i = 0uz; i < items.size(); i++)
			{
				auto& item = items[i];

				if (i > 0)
					_pVerticalSizer->AddSpacer(Spacing);

				fig::string timeString;
				if (item.timeBucket < TimeBucket::LessThan1Week)
					timeString = item.updatedAt.get_time_string(user_clock_setting);
				else
					timeString = item.updatedAt.get_date_string();

				auto pListItem = CreateControl<ChatListItem>(item.assetId, *item.chatLog, timeString);
				pListItem->SetDelegate([this](auto& card) { Reorder(); });
				item.pListItem = pListItem;

				_pVerticalSizer->Add(pListItem, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);
			}
		}

		if (_children.empty())
		{
			auto pHeader = CreateHeader("No chats");
			_pVerticalSizer->Add(pHeader, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);
		}

		InvalidateLayout();
	}
}