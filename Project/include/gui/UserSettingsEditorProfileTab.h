#pragma once

#include "gui/EditorTab.h"
#include "gui/UserSettingsEditorArgs.h"

namespace fig::gui
{
	class UserSettingsEditorProfileTab : public EditorTab<UserSettingsEditorArgs>
	{
	public:
		UserSettingsEditorProfileTab(ControlPtr pParent);

	protected:
		bool OnInit(UserSettingsEditorArgs args) override;

	protected:
		void OnUpdate(float fElapsed) override;
		void OnClose() noexcept override;
		void OnTabSelected() override;

		void GenerateRecoveryCode();
		void ChangePassword();
		void ClearPassword();
		void RefreshRecoveryCodeButton();
		void RefreshDeleteButton();
		
		void OnClickedProfileImage();
		void OpenFile();
		void OnOpenFile(const fig::path& filename);
		void ProcessLoadQueue();
		void RevertProfileImage();
		void RemoveProfileImage();
		void DeleteProfile();
		static void SDLCALL OnFileDialogResult(void* userdata, const char* const* fileList, int filter);

	private:
		fig::observer_ptr<TextBox> _pName;
		fig::observer_ptr<TextBox> _pOldPassword;
		fig::observer_ptr<TextBox> _pNewPassword;
		fig::observer_ptr<TextBox> _pConfirmPassword;
		fig::observer_ptr<ButtonWithLabel> _pRecoveryCodeButton;
		fig::observer_ptr<ButtonWithLabel> _pDeleteProfileButton;
		fig::observer_ptr<class ProfileImageWidget> _pProfileImage;
		std::array<fig::observer_ptr<CheckBox>, 3uz> _pConfirmCheckBoxes {};

		std::queue<fig::path> _profileImageLoadQueue;
		bool _bChangedProfileImage {};
		bool _bHasProfileImage {};
	};
}