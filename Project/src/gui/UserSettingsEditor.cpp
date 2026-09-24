#include <pch.h>
#include "gui/UserSettingsEditor.h"
#include "gui/UserSettingsEditorArgs.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr enum Tab : int32_t
	{
		Application = 0,
		Profile,
		Voice,
		Extensions,
		Advanced,
	};

	UserSettingsEditor::UserSettingsEditor(ControlPtr pParent) : Editor(pParent)
	{
	}

	fig::string UserSettingsEditor::GetTitle() const noexcept
	{
		return "Settings";
	}

	bool UserSettingsEditor::Initialize(const fig::uuid& assetId) noexcept
	{
		UserSettingsEditorArgs args = &Global::GetUserSettings();

		for (auto tab : _tabs | std::views::transform([](auto&& t) { return dynamic_cast<EditorTab<UserSettingsEditorArgs>*>(t.get()); }))
		{
			if (not (tab and tab->Initialize(args)))
				return false;
		}

		return true;
	}

	void UserSettingsEditor::PopulateTopBar(ControlPtr pParent)
	{
		auto pSizer = pParent->GetSizer();

		auto pSaveButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Apply", Resource::ICON_SAVE);
		pSaveButton->SetSize(110, 32);
		pSaveButton->SetDelegate([this] {
			Save();
		});
		_pSaveButton = pSaveButton;

		auto pDiscardButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Discard", Resource::ICON_DISMISS);
		pDiscardButton->SetSize(110, 32);
		pDiscardButton->SetDelegate([this] {
			PushEvent(UserEvent::NavigateToHome);
		});

		pSizer->Add(_pSaveButton, 0, SizerFlag::AlignCenterVertical);
		pSizer->Add(pDiscardButton, 0, SizerFlag::Left | SizerFlag::AlignCenterVertical, 8);
		pSizer->AddSpacer(8);
	}

	bool UserSettingsEditor::Save() noexcept
	{
		return true;
	}

	void UserSettingsEditor::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	std::vector<EditorTabDescriptor> UserSettingsEditor::GetTabDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorTabDescriptor> {
			{
				NotImpl,
					"Application",
					Resource::ICON_USER_SETTINGS_EDIT_APPLICATION,
					Resource::ICON_USER_SETTINGS_EDIT_APPLICATION_SMALL,
			},
			{
				NotImpl,
				"Profile",
				Resource::ICON_USER_SETTINGS_EDIT_PROFILE,
				Resource::ICON_USER_SETTINGS_EDIT_PROFILE_SMALL,
			},
			{
				NotImpl,
				"Voice",
				Resource::ICON_CHARACTER_EDIT_VOICE,
				Resource::ICON_CHARACTER_EDIT_VOICE_SMALL,
			},
			{
				NotImpl,
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
		_pSaveButton->SetTheme(SaveButtonStyle);
	}
}