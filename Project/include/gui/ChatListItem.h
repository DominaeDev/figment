#pragma once

#include "gui/Panel.h"

namespace fig::gui
{
	class ChatListItem : public Panel
	{
	public:
		ChatListItem(LayoutElement* pParent);

	protected:
		void OnSize() override;
		EventResult OnEvent(Event& event);

		void ShowMenu() noexcept;

	private:
		fig::observer_ptr<ImageWithMask> _pPortrait;
		fig::observer_ptr<StaticText> _pTitle;
		fig::observer_ptr<StaticText> _pMessage;
		fig::observer_ptr<StaticText> _pTimestamp;
	};
}