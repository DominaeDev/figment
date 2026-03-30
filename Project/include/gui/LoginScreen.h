#ifndef LOGIN_SCREEN_H__
#define LOGIN_SCREEN_H__
#pragma once

#include "Screen.h"
#include "GUICommon.h"

namespace fig::gui
{
	class PasswordBox;

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
		PasswordBox* _pPassword;
	};
}


#endif
