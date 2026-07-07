#pragma once

#include "gui/Screen.h"
#include "gui/GUICommon.h"

namespace fig::gui
{
	class PasswordBox;
	class ButtonWithLabel;

	class LoginScreen : public Screen
	{
	public:
		LoginScreen(Frame* pParent);

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;

	private:
		void SelectProfile(const fig::user::UserProfile& profile);
		void CycleProfile(int32_t step);
		bool SignIn();
		void ShowMenu();

	private:
		fig::observer_ptr<PasswordBox> _pPassword;
		fig::observer_ptr<ImageWithMask> _pProfileImage;
		fig::observer_ptr<StaticText> _pProfileName;
		fig::observer_ptr<ButtonWithIcon> _pPrevProfileBtn;
		fig::observer_ptr<ButtonWithIcon>_pNextProfileBtn;
		fig::observer_ptr<ButtonWithIcon>_pSignInBtn;
		fig::observer_ptr<ButtonWithIcon>_pMenuButton;
		fig::observer_ptr<ButtonWithLabel> _pNoPassButton;
		fig::observer_ptr<Panel> _pPasswordPanel;

		fig::uuid _currentProfileId {};
	};
}
