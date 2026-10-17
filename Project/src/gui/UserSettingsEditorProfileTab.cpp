#include <pch.h>
#include "gui/UserSettingsEditorProfileTab.h"
#include "gui/TextBox.h"
#include "gui/ButtonWithLabel.h"
#include "user/UserManager.h"
#include "user/UserProfile.h"

using namespace fig::auth;
using namespace fig::user;

namespace fig::gui
{
	UserSettingsEditorProfileTab::UserSettingsEditorProfileTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool UserSettingsEditorProfileTab::Initialize(UserSettingsEditorArgs args)
	{
		auto pSizer = SetSizer<VerticalSizer>();

		CreateLabel(this, pSizer, "Profile name");
		_pName = CreateTextBox(this, pSizer);
		_pName->SetMaxWidth(300);
		_pName->SetText(Global::GetUserManager().GetActiveProfile()->name);

		pSizer->AddSpacer(12);

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Change password");

		CreateLabel(this, pSizer, "Current password");
		_pOldPassword = CreateTextBox(this, pSizer);
		_pOldPassword->SetMaxWidth(260);
		_pOldPassword->SetMode(TextInput::Mode::Password);

		CreateLabel(this, pSizer, "New password");
		_pNewPassword = CreateTextBox(this, pSizer);
		_pNewPassword->SetMaxWidth(260);
		_pNewPassword->SetMode(TextInput::Mode::Password);

		CreateLabel(this, pSizer, "Confirm new password");
		_pConfirmPassword = CreateTextBox(this, pSizer);
		_pConfirmPassword->SetMaxWidth(260);
		_pConfirmPassword->SetMode(TextInput::Mode::Password);

		pSizer->AddSpacer(12);

		auto pChangeButton = CreateControl<ButtonWithLabel>("Change password");
		pChangeButton->SetHeight(35);
		pChangeButton->SetDelegate([this] { ChangePassword(); });
		pSizer->Add(pChangeButton, 0);
				
		// Recovery
		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Profile recovery");

		auto pHint = CreateHint(this, pSizer, "In the event that you forget your password, you can use a recovery code to reset it. Make sure you write it down and store it securely. When a new recovery code is generated, any prior code will be rendered invalid.");
		pHint->SetMaxWidth(480);

		pSizer->AddSpacer(52); //! @todo: hint height?

		_pRecoveryCodeButton = CreateControl<ButtonWithLabel>("Generate recovery code");
		_pRecoveryCodeButton->SetHeight(35);
		_pRecoveryCodeButton->SetDelegate([this] { GenerateRecoveryCode(); });

		pSizer->Add(_pRecoveryCodeButton, 0);

		CreateTabGroup();

		return true;
	}

	void UserSettingsEditorProfileTab::OnClose() noexcept
	{
		if (auto newName = trim(_pName->GetText()); not empty_or_whitespace(newName) and Global::GetUserManager().GetActiveProfile()->name != newName)
		{
			if (Global::GetUserManager().ChangeProfileName(Global::GetUserManager().GetActiveProfile()->id, fig::string { newName }))
				LogLn(std::format("Changed profile name to '{}'", newName));
			else
				LogLn("Failed to change name");
		}
	}

	void UserSettingsEditorProfileTab::OnTabSelected()
	{
		ClearPassword();
		RefreshRecoveryCodeButton();
	}

	void UserSettingsEditorProfileTab::GenerateRecoveryCode()
	{
		Global::GetUserManager().GenerateNewRecoveryKey();
	}

	void UserSettingsEditorProfileTab::ChangePassword()
	{
		auto oldPassword = fig::string { _pOldPassword->GetText() };
		auto newPassword = fig::string { _pNewPassword->GetText() };
		auto confirmPassword = fig::string { _pConfirmPassword->GetText() };

		if (newPassword != confirmPassword)
		{
			ShowDialogBox("Password conformation does not match.\nPlease try again.", DialogBoxType::Ok);
			ClearPassword();
			return;
		}

		if (auto profile = Global::GetUserManager().GetActiveProfile())
		{
			AuthKey tmp;
			if (not UserManager::Authenticate(*profile, oldPassword, tmp))
			{
				ShowDialogBox("Incorrect password.\nPlease try again.", DialogBoxType::Ok);
				ClearPassword();
				return;
			}

			if (auto code = Global::GetUserManager().ChangePassword((*profile).id, oldPassword, newPassword))
			{
				ShowDialogBox("Password has been successfully changed.", DialogBoxType::Ok);
				ClearPassword();
				RefreshRecoveryCodeButton();
				return;
			}
		}
	}

	void UserSettingsEditorProfileTab::ClearPassword()
	{
		if (_pOldPassword)
		{
			_pOldPassword->Clear();
			_pOldPassword->SetEnabled(Global::GetUserManager().GetActiveProfile()->state == UserProfile::State::Locked);
		}
		if (_pNewPassword)
			_pNewPassword->Clear();
		if (_pConfirmPassword)
			_pConfirmPassword->Clear();
	}

	void UserSettingsEditorProfileTab::RefreshRecoveryCodeButton()
	{
		if (_pRecoveryCodeButton)
			_pRecoveryCodeButton->SetEnabled(Global::IsSignedIn() and Global::GetUserManager().GetActiveProfile()->state == UserProfile::State::Locked);
	}

}