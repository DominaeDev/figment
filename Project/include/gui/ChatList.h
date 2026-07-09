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
		void Reset();

	protected:
		void OnScroll() override;
		void OnAfterLayout() override;
		ControlPtr CreateHeader(fig::string_view text);

	private:
		std::vector<fig::observer_ptr<ChatListItem>> _items;

		fig::observer_ptr<VerticalSizer> _pVerticalSizer;
	};
}