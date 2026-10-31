#pragma once

#include "Control.h"
#include "MouseEventHandler.h"

namespace fig::gui
{
	class StoryRoleSlotBox : public Control, public MouseEventHandler
	{
	public:
		StoryRoleSlotBox(ControlPtr pParent);

		void SetRole(fig::chat::Role role, fig::string_view label);

	protected:
		void OnSize() override;
		EventResult OnEvent(fig::event& event) override;

	private:
		fig::observer_ptr<StaticText> _pLabel {};
		fig::observer_ptr<Image> _pIcon {};
	};
}