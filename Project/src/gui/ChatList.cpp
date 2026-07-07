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

	ChatList::ChatList(LayoutElement* pParent) : ScrollPanel(pParent)
	{
		_pVerticalSizer = new VerticalSizer();
		SetTopMargin(TopMargin);
		SetBottomMargin(BottomMargin);

		SetSizer(_pVerticalSizer);
		EnableClipping(true);
		EnableCulling(true);
	}

	void ChatList::ShowAllChats()
	{
		Reset();

		auto& assetMngr = Global::GetUserManager().GetContent().GetAssetManager();
		
		for (int i = 0; i < 30; i++) //! @temp
		{
			if (i > 0)
				_pVerticalSizer->AddSpacer(Spacing);

			if (i % 6 == 0) 
			{
				auto pHeader = CreateHeader("Yesterday");
				_pVerticalSizer->Add(pHeader, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);
			}

			auto pItem = CreateControl<ChatListItem>();
			_pVerticalSizer->Add(pItem, 0, Sizer::AlignCenterHorizontal | Sizer::Expand | Sizer::Right, 18);
			_items.push_back(pItem);
		}

		InvalidateLayout();
	}

	ControlPtr ChatList::CreateHeader(const fig::string& text)
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