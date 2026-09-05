#pragma once

#include "gui/SidePanel.h"

namespace fig::gui
{
	class ButtonWithIcon;
	class UserProfileWidget;
	class LoadModelWidget;

	class SidePanelMain : public SidePanelContent
	{
	public:
		SidePanelMain(ControlPtr pParent);

		void ShowExpanded() override;
		void ShowCollapsed() override;

	protected:
		EventResult OnEvent(fig::event& event) override;
		void ShowMenu();

	private:
		fig::observer_ptr<UserProfileWidget> _pUserWidget;
		fig::observer_ptr<LoadModelWidget> _pModelWidget;
		fig::observer_ptr<ButtonWithIcon> _pMenuButton;
	};
}