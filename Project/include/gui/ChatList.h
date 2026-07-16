#pragma once

#include "gui/ScrollPanel.h"

namespace fig::gui
{
	class VerticalSizer;
	class ChatListItem;

	class ChatList : public ScrollPanel
	{
	public:
		ChatList(ParentPtr pParent);

		void ShowAllChats();
		void ShowChatsWith(const fig::uuid& characterId);

	protected:
		void Reset();
		void OnScroll() override;
		void OnAfterLayout() override;
		ControlPtr CreateHeader(fig::string_view text);

		void ShowChats(const fig::cref_vector<fig::io::Asset>& chats);

	private:
		std::vector<fig::observer_ptr<ChatListItem>> _items;

		fig::observer_ptr<VerticalSizer> _pVerticalSizer;
	};
}