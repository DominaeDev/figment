#pragma once

#include "gui/EditorTab.h"
#include "gui/UserSettingsEditorArgs.h"

namespace fig::gui
{
	class UserSettingsEditorProfileTab : public EditorTab<UserSettingsEditorArgs>
	{
	public:
		UserSettingsEditorProfileTab(ControlPtr pParent);

		bool Initialize(UserSettingsEditorArgs args) override;

	protected:
		void OnClose() noexcept override;
		void OnTabSelected() override;

		void GenerateRecoveryCode();
		void ChangePassword();
		void ClearPassword();
		void RefreshRecoveryCodeButton();

	private:
		fig::observer_ptr<TextBox> _pName;
		fig::observer_ptr<TextBox> _pOldPassword;
		fig::observer_ptr<TextBox> _pNewPassword;
		fig::observer_ptr<TextBox> _pConfirmPassword;
		fig::observer_ptr<ButtonWithLabel> _pRecoveryCodeButton;
	};
}