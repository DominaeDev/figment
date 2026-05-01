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
	class LoadModelWidget;

	class SidePanel : public Control
	{
	public:
		SidePanel(LayoutElement* pParent);
	
	protected:
		void OnAfterLayout() override;
		bool OnEvent(Event& event) override;
		void ShowMenu();

	private:
		LayoutElement* _pGradient;
		UserProfileWidget* _pUserWidget;
		LoadModelWidget* _pModelWidget;
		ButtonWithIcon* _pMenuButton;
	};
}
#endif