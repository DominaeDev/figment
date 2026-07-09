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

	enum class TimeBucket
	{
		LessThan5Minutes = 0,
		LessThan1Day,
		LessThan2Days,
		LessThan1Week,
		LessThan1Month,
		Older,
	};

	static std::array<fig::string_view, 6> TimeBucketNames {
		"Just now",
		"Today",
		"Yesterday",
		"This week",
		"This month",
		"Older chats",
	};

	static TimeBucket GetTimeBucket(fig::timestamp then, fig::timestamp now)
	{
		auto minDiff = (now.to_local() - then.to_local()).minutes();
		auto dayDiff = (now.to_local() - then.to_local()).days();
		auto monthDiff = (now.to_local() - then.to_local()).months();

		if (minDiff < 10)
			return TimeBucket::LessThan5Minutes;
		else if (dayDiff < 1)
			return TimeBucket::LessThan1Day;
		else if (dayDiff < 2)
			return TimeBucket::LessThan2Days;
		else if (dayDiff < 7)
			return TimeBucket::LessThan1Week;
		else if (monthDiff < 1)
			return TimeBucket::LessThan1Month;
		return TimeBucket::Older;
	}

	void ChatList::ShowAllChats()
	{
		Reset();

		auto chatInstances = Global::GetUserManager().GetContent().GetChats(true)
			| std::views::transform([](auto& a) { return std::cref(a); })
			| std::ranges::to<std::vector>();

		auto now = utc_now();
		auto chatsByTime = chatInstances
			| fig::group_by([&now](auto& a) { return GetTimeBucket(a.get().GetCreatedAt(), now); });
		
		for (auto& kvp : chatsByTime)
		{
			if (not _items.empty())
				_pVerticalSizer->AddSpacer(Spacing);

			// Header
			auto pHeader = CreateHeader(TimeBucketNames[static_cast<size_t>(kvp.first)]);
			_pVerticalSizer->Add(pHeader, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);

			// Chats
			auto& chats = kvp.second;
			for (size_t i = 0uz; i < chats.size(); i++)
			{
				if (i > 0)
					_pVerticalSizer->AddSpacer(Spacing);

				auto pItem = CreateControl<ChatListItem>(chats[i]);
				_pVerticalSizer->Add(pItem, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);
				_items.push_back(pItem);
			}
		}

		InvalidateLayout();
	}

	ControlPtr ChatList::CreateHeader(fig::string_view text)
	{
		auto panel = CreateControl<Area>();
		panel->SetMaxSize(700, -1);
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