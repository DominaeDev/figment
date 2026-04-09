#ifndef SIDE_PANEL_H__
#define SIDE_PANEL_H__
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

	class SidePanel : public Control
	{
	public:
		SidePanel(LayoutElement* pParent);
	
		void SetUserProfile(const fig::user::UserProfile& profile);
		void Reset();

	protected:
		void OnAfterLayout() override;
		void ShowMenu();

	private:
		LayoutElement* _pGradient;
		UserProfileWidget* _pUserWidget;
		ButtonWithIcon* _pMenuButton;
	};
}
#endif