#pragma once

#include "Control.h"
#include "MouseEventHandler.h"

namespace fig::gui
{
	class AddImageButton : public Control, public MouseEventHandler
	{
	public:
		AddImageButton(ControlPtr pParent, fig::string_view label);

	protected:
		void OnMouseEnter() override;
		void OnMouseExit() override;
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;
		void OnClicked() override;

	private:
		fig::observer_ptr<StaticText> _pLabel {};
	};
}