#pragma once

#include "gui/ScrollPanel.h"
#include "io/ContentManager.h"

namespace fig::gui
{
	class VerticalSizer;
	class ChatListItem;

	enum class ChatListItemEvent;

	class ChatList : public ScrollPanel
	{
	public:
		ChatList(ControlPtr pParent);

		void ShowAllChats();
		void ShowChatsWith(const fig::uuid& characterId);

		void SetFilter(const fig::string& filter) noexcept;
		void Reorder();
		void Clear();

	protected:
		void OnScroll() override;
		fig::coord GetExtent() const override;

		fig::observer_ptr<Control> CreateHeader(fig::string_view text);
		void OnItemEvent(ChatListItem& item, ChatListItemEvent event);

		void ShowChats(const fig::io::ChatCollection& chatInstances);
		void DeleteChat(ChatListItem& item);
		void DeleteArchivedChats();

	private:
		void Sort(fig::io::SortBy sortBy, fig::io::OrderBy orderBy);
		void Filter(fig::io::ChatFilterFlags filterBy, const fig::string& search_string);
		
		enum class TimeBucket
		{
			LessThan5Minutes = 0,
			LessThan1Day,
			LessThan2Days,
			LessThan1Week,
			LessThan1Month,
			Older,
		};

		struct Item
		{
			fig::uuid chatInstanceId;
			fig::uuid chatLogId;
			fig::timestamp createdAt;
			fig::timestamp updatedAt;
			fig::observer_ptr<ChatListItem> pListItem;
			std::unordered_set<fig::string> participants;
			TimeBucket timeBucket;
			bool filtered { false };

			bool FindParticipant(const SearchQuery& query) noexcept;
		};

		static TimeBucket GetTimeBucket(fig::timestamp then, fig::timestamp now) noexcept;

		std::vector<Item> _items;
		fig::observer_ptr<VerticalSizer> _pVerticalSizer;
		std::vector<fig::observer_ptr<Control>> _labels;
		fig::string _filterString;

	};
}