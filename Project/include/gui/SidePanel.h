#pragma once

#include "gui/Control.h"

namespace fig::user
{
	struct UserProfile;
}

namespace fig::gui
{
	class ButtonWithIcon;
	class UserProfileWidget;
	class LoadModelWidget;

	class SidePanel : public Control
	{
	public:
		SidePanel(control_ptr pParent);
	
		void Expand() noexcept;
		void Collapse() noexcept;

	protected:
		void OnAfterLayout() override;
		EventResult OnEvent(fig::event& event) override;
		void ShowMenu();

	private:
		fig::observer_ptr<LayoutElement> _pGradient;
		fig::observer_ptr<UserProfileWidget> _pUserWidget;
		fig::observer_ptr<LoadModelWidget> _pModelWidget;
		fig::observer_ptr<ButtonWithIcon> _pMenuButton;
		fig::observer_ptr<ButtonWithIcon> _pCollapseButton;
		bool _bExpanded { true };
		
		fig::observer_ptr<Control> _pExpandedRoot;
		fig::observer_ptr<Control> _pCollapsedRoot;
	};
}
