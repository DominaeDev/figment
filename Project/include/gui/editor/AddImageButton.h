#pragma once

#include "gui/Control.h"
#include "gui/MouseEventHandler.h"

namespace fig::gui
{
	class AddImageButton : public Control, public MouseEventHandler
	{
	public:
		AddImageButton(ControlPtr pParent);

	protected:
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;
		void OnMouseEnter() override;
		void OnMouseExit() override;
		void OnClicked() override;

	private:
		fig::observer_ptr<Image> _pIcon {};
	};
}