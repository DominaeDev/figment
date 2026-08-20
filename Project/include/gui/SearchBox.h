#pragma once

#include "gui/TextBox.h"
#include "gui/MouseEventHandler.h"

namespace fig::gui
{
	class SearchBox : public TextBox, public MouseEventHandler
	{
	public:
		SearchBox(ControlPtr pParent);

	protected:
		void OnSize();
		void OnText(fig::string_view text) noexcept override;
		EventResult OnEvent(fig::event& event);
		void OnEnabled(bool bEnabled) override;
		void OnClicked() override;

	private:
		fig::observer_ptr<Image> _pIcon;
		fig::observer_ptr<Image> _pCross;

		bool _bHasText { false };
	};
}