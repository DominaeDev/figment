#ifndef LOGIN_SCREEN_H__
#define LOGIN_SCREEN_H__
#pragma once

#include "Screen.h"
#include "GUICommon.h"

namespace fig::gui
{
	class PasswordBox;
	class ButtonWithLabel;

	class LoginScreen : public Screen
	{
	public:
		LoginScreen(Frame* pParent);

		SCREEN_ID(EScreen::Login);
	
	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;

	private:
		void SelectProfile(const fig::user::UserProfile& profile);
		void CycleProfile(int32_t step);
		bool SignIn();

	private:
		PasswordBox* _pPassword;
		ImageWithMask* _pProfileImage;
		StaticText* _pProfileName;
		ButtonWithIcon* _pPrevProfileBtn;
		ButtonWithIcon* _pNextProfileBtn;
		ButtonWithIcon* _pSignInBtn;
		ButtonWithLabel* _pNoPassButton;
		Panel* _pPasswordPanel {};
		fig::uuid _currentProfileId {};
	};
}


#endif
