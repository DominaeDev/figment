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

	ChatList::ChatList(ParentPtr pParent) : ScrollPanel(pParent)
	{
		_pVerticalSizer = SetSizer<VerticalSizer>();
		SetTopMargin(TopMargin);
		SetBottomMargin(BottomMargin);

		EnableClipping(true);
		EnableCulling(true);
	}

	static ChatListItem::TimeBucket GetTimeBucket(fig::timestamp then, fig::timestamp now)
	{
		auto minDiff = (now.to_local() - then.to_local()).minutes();
		auto dayDiff = (now.to_local() - then.to_local()).days();
		auto monthDiff = (now.to_local() - then.to_local()).months();

		if (minDiff < 10)
			return ChatListItem::TimeBucket::LessThan5Minutes;
		else if (dayDiff < 1)
			return ChatListItem::TimeBucket::LessThan1Day;
		else if (dayDiff < 2)
			return ChatListItem::TimeBucket::LessThan2Days;
		else if (dayDiff < 7)
			return ChatListItem::TimeBucket::LessThan1Week;
		else if (monthDiff < 1)
			return ChatListItem::TimeBucket::LessThan1Month;
		return ChatListItem::TimeBucket::Older;
	}

	void ChatList::ShowAllChats()
	{
		Reset();

		auto& content = Global::GetUserManager().GetContent();
		auto chatInstances = content.GetChatLogs(true)
			| std::views::transform([](auto& a) { return std::cref(a); })
			| std::ranges::to<std::vector>();

		auto now = utc_now();
		auto chatsByTime = chatInstances
			| std::views::transform([&content](auto& a) {
				auto chat = content.Get<fig::data::ChatLog>(a.get().id);
				return std::make_pair(chat, a.get().GetCreatedAt());
			})
			| fig::group_by([&now](auto& p) { return GetTimeBucket(p.second, now); });
		
		for (auto& kvp : chatsByTime)
		{
			if (not _items.empty())
				_pVerticalSizer->AddSpacer(Spacing);

			// Header
			auto pHeader = CreateHeader(ChatListItem::TimeBucketLabels[static_cast<size_t>(kvp.first)]);
			_pVerticalSizer->Add(pHeader, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);

			// Chats
			auto& chats = kvp.second;
			for (size_t i = 0uz; i < chats.size(); i++)
			{
				auto& chat = *chats[i].first;
				auto& time = chats[i].second;

				if (i > 0)
					_pVerticalSizer->AddSpacer(Spacing);

				auto pItem = CreateControl<ChatListItem>(chat, time, kvp.first);
				_pVerticalSizer->Add(pItem, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);
				_items.push_back(pItem);
			}
		}

		InvalidateLayout();
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
	
}