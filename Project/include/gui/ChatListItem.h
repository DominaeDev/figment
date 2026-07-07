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
		ImageWithMask* _pPortrait {};
		StaticText* _pTitle;
		StaticText* _pMessage;
		StaticText* _pTimestamp;
	};
}