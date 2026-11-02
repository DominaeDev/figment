#include <pch.h>
#include "gui/editor/UserSettingsEditor.h"
#include "gui/editor/UserSettingsEditorArgs.h"
#include "gui/editor/UserSettingsEditorMainTab.h"
#include "gui/editor/UserSettingsEditorProfileTab.h"
#include "gui/editor/UserSettingsEditorVoiceTab.h"
#include "gui/editor/UserSettingsEditorExtensionsTab.h"
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

		auto pDiscardButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Close", Icon::DISMISS);
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
				Icon::USER_SETTINGS_EDIT_APPLICATION,
				Icon::USER_SETTINGS_EDIT_APPLICATION_SMALL,
			},
			{
				Profile,
				"Profile",
				Icon::USER_SETTINGS_EDIT_PROFILE,
				Icon::USER_SETTINGS_EDIT_PROFILE_SMALL,
			},
			{
				Voice,
				"Voice",
				Icon::CHARACTER_EDIT_VOICE,
				Icon::CHARACTER_EDIT_VOICE_SMALL,
			},
			{
				Extensions,
				"Extensions",
				Icon::CHARACTER_EDIT_CONCEPTS,
				Icon::CHARACTER_EDIT_CONCEPTS_SMALL,
			},
			{
				NotImpl,
				"Advanced",
				Icon::USER_SETTINGS_EDIT_ADVANCED,
				Icon::USER_SETTINGS_EDIT_ADVANCED_SMALL,
			},
		};
	}

	void UserSettingsEditor::OnPropertyChanged()
	{
	}
}