#pragma once

#include "gui/ScrollPanel.h"

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

	protected:
		void OnScroll() override;
		fig::coord GetExtent() const override;

		fig::observer_ptr<Control> CreateHeader(fig::string_view text);
		void OnItemEvent(ChatListItem& item, ChatListItemEvent event);

		void ShowChats(const fig::cref_vector<fig::io::Asset>& chats);
		void Reset();
		void DeleteChat(ChatListItem& item);
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
			fig::uuid assetId;
			fig::optional_cref<class fig::data::ChatLog> chatLog;
			fig::timestamp createdAt;
			fig::timestamp updatedAt;
			fig::observer_ptr<ChatListItem> pListItem;
			TimeBucket timeBucket;
			bool filtered { false };

			bool MatchesFlags(fig::io::ChatFilterFlags filter) noexcept;
		};

		static TimeBucket GetTimeBucket(fig::timestamp then, fig::timestamp now) noexcept;

		std::vector<Item> _items;
		fig::observer_ptr<VerticalSizer> _pVerticalSizer;
		fig::string _filterString;

	};
}