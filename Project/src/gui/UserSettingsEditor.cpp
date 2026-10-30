#include <pch.h>
#include "gui/UserSettingsEditor.h"
#include "gui/UserSettingsEditorArgs.h"
#include "gui/UserSettingsEditorMainTab.h"
#include "gui/UserSettingsEditorProfileTab.h"
#include "gui/UserSettingsEditorVoiceTab.h"
#include "gui/UserSettingsEditorExtensionsTab.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"
#include "user/UserManager.h"

namespace fig::gui
{
	constexpr enum Tab : int32_t
	{
		Application,
		Profile,
		Voice,
		Extensions,
		// Advanced,
	};

	UserSettingsEditor::UserSettingsEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreateTab<UserSettingsEditorMainTab>();
		CreateTab<UserSettingsEditorProfileTab>();
		CreateTab<UserSettingsEditorVoiceTab>();
		CreateTab<UserSettingsEditorExtensionsTab>();
	}

	fig::string UserSettingsEditor::GetTitle() const noexcept
	{
		return "Settings";
	}

	bool UserSettingsEditor::Initialize(const fig::uuid& assetId) noexcept
	{
		UserSettingsEditorArgs args = &Global::GetUserSettings();

		InitTabs(args);
		return true;
	}

	void UserSettingsEditor::PopulateTopBar(ControlPtr pParent)
	{
		auto pSizer = pParent->GetSizer();

		auto pDiscardButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Close", Resource::ICON_DISMISS);
		pDiscardButton->SetSize(110, 32);
		pDiscardButton->SetDelegate([this] { Close(); });

		pSizer->Add(pDiscardButton, 0, SizerFlag::Left | SizerFlag::AlignCenterVertical, 8);
		pSizer->AddSpacer(8);
	}

	void UserSettingsEditor::Close()
	{
		if (Global::GetUserManager().IsSignedIn())
		{
			ClearDirtyFlag();
			Editor::Close();
			PushEvent(UserEvent::UserSettingsChanged);
		}
	}

	std::vector<EditorTabDescriptor> UserSettingsEditor::GetTabDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorTabDescriptor> {
			{
				Application,
				"Application",
				Resource::ICON_USER_SETTINGS_EDIT_APPLICATION,
				Resource::ICON_USER_SETTINGS_EDIT_APPLICATION_SMALL,
			},
			{
				Profile,
				"Profile",
				Resource::ICON_USER_SETTINGS_EDIT_PROFILE,
				Resource::ICON_USER_SETTINGS_EDIT_PROFILE_SMALL,
			},
			{
				Voice,
				"Voice",
				Resource::ICON_CHARACTER_EDIT_VOICE,
				Resource::ICON_CHARACTER_EDIT_VOICE_SMALL,
			},
			{
				Extensions,
				"Extensions",
				Resource::ICON_CHARACTER_EDIT_CONCEPTS,
				Resource::ICON_CHARACTER_EDIT_CONCEPTS_SMALL,
			},
			{
				NotImpl,
				"Advanced",
				Resource::ICON_USER_SETTINGS_EDIT_ADVANCED,
				Resource::ICON_USER_SETTINGS_EDIT_ADVANCED_SMALL,
			},
		};
	}

	void UserSettingsEditor::OnPropertyChanged()
	{
	}
}