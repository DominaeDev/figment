#include <pch.h>
#include "user/UserManager.h"
#include "user/UserProfile.h"
#include "gui/UserSettingsEditorProfileTab.h"
#include "gui/TextBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/RecoveryCodeModal.h"
#include "gui/CheckBox.h"
#include "gui/Frame.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"
#include "gui/ProfileImageWidget.h"
#include "gui/MainFrame.h"
#include "io/FileUtility.h"

using namespace fig::auth;
using namespace fig::user;
using namespace fig::strings;
using namespace fig::io;

namespace fig::gui
{
	UserSettingsEditorProfileTab::UserSettingsEditorProfileTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool UserSettingsEditorProfileTab::Initialize(UserSettingsEditorArgs args)
	{
		auto pSizer = SetSizer<VerticalSizer>();

		auto& profile = Global::GetUserManager().GetActiveProfile().value();

		CreateHeader(this, pSizer, "User profile");

		auto pHSizer = new HorizontalSizer();
		auto pColumn1 = new VerticalSizer();
		auto pColumn2 = new VerticalSizer();
		pHSizer->Add(pColumn1, 0, SizerFlag::FixedSize, 140);
		pHSizer->Add(pColumn2, -1);

		CreateLabel(this, pColumn1, "Profile picture");
		_pProfileImage = CreateControl<ProfileImageWidget>();
		_pProfileImage->SetRightClickDelegate([this]() { OnClickedProfileImage(); });
		pColumn1->Add(_pProfileImage, 0);
		
		RevertProfileImage();

		CreateLabel(this, pColumn2, "Profile name");
		_pName = CreateTextBox(this, pColumn2);
		_pName->SetMaxWidth(300);
		_pName->SetText(profile.name);

		pSizer->Add(pHSizer, 0, SizerFlag::FixedSize, 156);

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

		_pRecoveryCodeButton = CreateControl<ButtonWithLabel>("Generate recovery code");
		_pRecoveryCodeButton->SetHeight(35);
		_pRecoveryCodeButton->SetDelegate([this] { GenerateRecoveryCode(); });
		pSizer->Add(_pRecoveryCodeButton, 0);
		pSizer->AddSpacer(4);

		auto pHint = CreateHint(this, pSizer, ProfileSettings::RecoveryCodeHint);
		pHint->SetMaxWidth(480);

		pSizer->AddSpacer(46); //! @todo: hint height?

		// Delete account

		CreateHorizontalLine(this, pSizer);
		CreateHeader(this, pSizer, "Danger zone");

		_pDeleteProfileButton = CreateControl<ButtonWithLabel>("Delete profile");
		_pDeleteProfileButton->SetHeight(35);
		_pDeleteProfileButton->SetDelegate([this] { DeleteProfile(); });
		pSizer->Add(_pDeleteProfileButton, 0, SizerFlag::Bottom, 4);

		_pConfirmCheckBoxes[0] = CreateControl<CheckBox>("Yes, I want to delete this profile");
		_pConfirmCheckBoxes[1] = CreateControl<CheckBox>("I am positively sure");
		_pConfirmCheckBoxes[2] = CreateControl<CheckBox>("And I will not regret it");
		for (auto& pCB : _pConfirmCheckBoxes)
		{
			pCB->SetDelegate([this](bool b) { RefreshDeleteButton(); });
			pSizer->Add(pCB, 0);
		}
		pSizer->AddSpacer(8);

		CreateTabGroup();
		return true;
	}

	void UserSettingsEditorProfileTab::OnUpdate(float fElapsed)
	{
		// Load images in load queue
		ProcessLoadQueue();
	}

	void UserSettingsEditorProfileTab::OnClose() noexcept
	{
		// If we're signed out here, the user has probably deleted the profile and all changes are moot.
		if (not Global::GetUserManager().IsSignedIn())
			return; 

		if (auto profile = Global::GetUserManager().GetActiveProfile())
		{
			if (auto newName = trim(_pName->GetText()); not empty_or_whitespace(newName) and Global::GetUserManager().GetActiveProfile()->name != newName)
			{
				if (Global::GetUserManager().ChangeProfileName((*profile).id, fig::string { newName }))
					LogLn(std::format("Changed profile name to '{}'", newName));
				else
					LogLn("Failed to change name");
			}

			if (_bChangedProfileImage)
			{
				if (_bHasProfileImage)
				{
					if (auto profileImage = _pProfileImage->GetImage(Constants::Data::ProfileImageWidth, Constants::Data::ProfileImageHeight))
					{
						if (Global::GetUserManager().ChangeProfilePicture(*profile, profileImage) == FileError::NoError)
							LogLn("Changed profile image");
						else
							LogLn("Failed to change profile image");

					}
				}
				else
				{
					if (Global::GetUserManager().ChangeProfilePicture(*profile, {}) == FileError::NoError)
						LogLn("Removed profile image");
					else
						LogLn("Failed to change profile image");
				}
			}
		}
	}

	void UserSettingsEditorProfileTab::OnTabSelected()
	{
		ClearPassword();
		RefreshRecoveryCodeButton();

		for (auto& pCB : _pConfirmCheckBoxes)
			pCB->SetValue(false, true);
		RefreshDeleteButton();
	}

	void UserSettingsEditorProfileTab::GenerateRecoveryCode()
	{
		if (auto code = Global::GetUserManager().GenerateNewRecoveryKey())
		{
			auto& dlg = GetOwnerFrame()->CreateModal<RecoveryCodeModal>(*code);
			dlg.Show();
		}
	}

	void UserSettingsEditorProfileTab::ChangePassword()
	{
		auto oldPassword = fig::string { _pOldPassword->GetText() };
		auto newPassword = fig::string { _pNewPassword->GetText() };
		auto confirmPassword = fig::string { _pConfirmPassword->GetText() };

		if (newPassword != confirmPassword)
		{
			ShowDialogBox(ProfileSettings::ConfirmationFailedError, DialogBoxType::Ok);
			ClearPassword();
			return;
		}

		if (auto profile = Global::GetUserManager().GetActiveProfile())
		{
			AuthKey tmp;
			if (not UserManager::Authenticate(*profile, oldPassword, tmp))
			{
				ShowDialogBox(ProfileSettings::IncorrectPasswordError, DialogBoxType::Ok);
				ClearPassword();
				return;
			}

			if (auto code = Global::GetUserManager().ChangePassword((*profile).id, oldPassword, newPassword))
			{
				ShowDialogBox(ProfileSettings::PasswordChanged, DialogBoxType::Ok);
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

	void UserSettingsEditorProfileTab::RefreshDeleteButton()
	{
		if (not _pConfirmCheckBoxes[0]->GetValue())
			_pConfirmCheckBoxes[1]->SetValue(false, true);
		if (not _pConfirmCheckBoxes[1]->GetValue())
			_pConfirmCheckBoxes[2]->SetValue(false, true);
		_pConfirmCheckBoxes[1]->SetEnabled(_pConfirmCheckBoxes[0]->GetValue());
		_pConfirmCheckBoxes[2]->SetEnabled(_pConfirmCheckBoxes[1]->GetValue());
		_pDeleteProfileButton->SetEnabled(_pConfirmCheckBoxes[0]->GetValue() and _pConfirmCheckBoxes[1]->GetValue() and _pConfirmCheckBoxes[2]->GetValue());
	}

	void SDLCALL UserSettingsEditorProfileTab::OnFileDialogResult(void* userdata, const char* const* fileList, int filter)
	{
		if (not fileList)
			return;

		auto pThis = static_cast<UserSettingsEditorProfileTab*>(userdata);
		for (const char* const* path = fileList; *path; ++path)
			pThis->OnOpenFile(std::filesystem::u8path(*path));
	}

	void UserSettingsEditorProfileTab::OpenFile()
	{
		static constexpr SDL_DialogFileFilter filters[] =
		{
			{ "Image files", "bmp;gif;jpg;jpeg;jfif;png;webp" },
			{ "BMP files", "bmp" },
			{ "GIF files", "gif" },
			{ "JPEG files", "jpg;jpeg;jfif" },
			{ "PNG files", "png" },
			{ "WEBP files", "webp" },
		};

		SDL_ShowOpenFileDialog(OnFileDialogResult, (void*)this, GetSDLWindow(), filters, SDL_arraysize(filters), nullptr, false);
	}

	void UserSettingsEditorProfileTab::OnOpenFile(const fig::path& filename)
	{
		_profileImageLoadQueue.push(filename);
	}

	void UserSettingsEditorProfileTab::OnClickedProfileImage()
	{
		auto& menu = CreateMenu();
		menu.AddItem("Load image\u2026")
			.SetDelegate([this] { OpenFile(); });
		menu.AddSeparator();
		menu.AddItem("Revert")
			.SetDelegate([this] { RevertProfileImage(); })
			.SetEnabled(_bChangedProfileImage);
		menu.AddItem("Remove")
			.SetDelegate([this] { RemoveProfileImage(); })
			.SetEnabled(_bHasProfileImage);
		menu.Show();
	}

	void UserSettingsEditorProfileTab::ProcessLoadQueue()
	{
		if (_profileImageLoadQueue.empty())
			return;

		bool bChanged = false;

		// Load small portrait
		if (not _profileImageLoadQueue.empty())
		{
			auto& path = _profileImageLoadQueue.front();
			auto data = fig::io::ReadFile(path).value_or({});
			if (auto try_surface = LoadImageFromMemory(data); try_surface.has_value() and not data.empty())
			{
				_pProfileImage->SetImage(*try_surface);
				_bHasProfileImage = true;
				_bChangedProfileImage = true;
				bChanged = true;
			}
			queue_clear(_profileImageLoadQueue);
		}
	}

	void UserSettingsEditorProfileTab::RevertProfileImage()
	{
		auto& profile = Global::GetUserManager().GetActiveProfile().value();

		if (auto profileImage = UserManager::GetProfileImage(profile))
		{
			_pProfileImage->SetImage(profileImage);
			_bHasProfileImage = true;
		}
		else
		{
			_pProfileImage->SetTexture(AppResources::GetTexture(Resource::PROFILE_DEFAULT_IMAGE));
			_bHasProfileImage = false;
		}
		_pProfileImage->SetEditable(false);
		_bChangedProfileImage = false;
	}

	void UserSettingsEditorProfileTab::RemoveProfileImage()
	{
		auto& profile = Global::GetUserManager().GetActiveProfile().value();

		_pProfileImage->SetTexture(AppResources::GetTexture(Resource::PROFILE_DEFAULT_IMAGE));
		_pProfileImage->SetEditable(false);
		_bHasProfileImage = false;
		_bChangedProfileImage = true;
	}

	void UserSettingsEditorProfileTab::DeleteProfile()
	{
		ShowDialogBox("Are you really sure you want to permanently delete the profile?\nThis action cannot be undone.", DialogBoxType::YesNo,
			[this](bool bOk) {
				if (bOk)
				{
					fig::uuid profileId = (*Global::GetUserManager().GetActiveProfile()).id;
					if (Global::GetUserManager().DeleteProfile(profileId))
						LogLn(std::format("Deleted profile {}", (fig::string)profileId));
					PushEvent(UserEvent::ProfileDeleted);
				}
				else
				{
					// Uncheck boxes
					for (auto& pCB : _pConfirmCheckBoxes)
						pCB->SetValue(false, true);
					RefreshDeleteButton();
				}
			});
	}

}