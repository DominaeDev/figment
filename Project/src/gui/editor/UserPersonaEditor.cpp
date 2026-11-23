#include <pch.h>
#include "gui/editor/UserPersonaEditor.h"
#include "gui/editor/UserPersonaEditorGeneralTab.h"
#include "gui/editor/CharacterEditorImagesTab.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/DialogBox.h"
#include "gui/Frame.h"
#include "gui/AppResources.h"
#include "io/ContentManager.h"

using namespace fig::io;

namespace fig::gui
{
	constexpr enum Tab : int32_t
	{
		General = 0,
	};

	UserPersonaEditor::UserPersonaEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreateTab<UserPersonaEditorGeneralTab>();
	}

	fig::string UserPersonaEditor::GetTitle() const noexcept
	{
		return "Editing identity";
	}
	
	bool UserPersonaEditor::NewCharacter(const fig::uuid& folderId) noexcept
	{
		auto& asset = Global::GetUserContent().GetAssets().CreateTemporaryAsset(make_asset_type(AssetType::UserPersona, DataFormat::TextXml), fig::uuid {}, folderId);

		_assetId = asset.id;
		_character = {};

		CharacterEditorArgs args
		{
			.assetId = _assetId,
			.pCharacter = &_character,
		};

		InitTabs(args);
		return true;
	}

	bool UserPersonaEditor::EditCharacter(const fig::uuid& assetId) noexcept
	{
		if (auto try_character = Global::GetUserContent().Get<fig::data::UserPersona>(assetId))
		{
			_assetId = assetId;
			_character = fig::data::UserPersona { *try_character };
			_bEditing = true;
			CharacterEditorArgs args
			{
				.assetId = _assetId,
				.pCharacter = &_character,
			};

			InitTabs(args);
			return true;
		}

		_assetId = {};
		return false;
	}

	void UserPersonaEditor::PopulateTopBar(ControlPtr pParent)
	{
		auto pSizer = pParent->GetSizer();

		auto pSaveButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Save", Icon::SAVE);
		pSaveButton->SetSize(110, 32);
		pSaveButton->SetDelegate([this] { SaveChanges(); });
		_pSaveButton = pSaveButton;

		auto pDiscardButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Cancel", Icon::DISMISS);
		pDiscardButton->SetSize(110, 32);
		pDiscardButton->SetDelegate([this] { Close(); });

		pSizer->Add(_pSaveButton, 0, SizerFlag::AlignCenterVertical);
		pSizer->Add(pDiscardButton, 0, SizerFlag::Left | SizerFlag::AlignCenterVertical, 8);
		pSizer->AddSpacer(8);
	}

	bool UserPersonaEditor::Save() noexcept
	{
		if (_assetId.empty())
			return false;

		bool bOk = SaveTabs();

		_character.RebuildSearchIndex();

		if (not bOk)
		{
			LogLn("Error occurred while saving user persona."); //! @todo: User facing error
			return false; // Error
		}
		
		// If this is the first user persona, mark it as the default
		if (not _bEditing and Global::GetUserContent().GetAssets().FindAssetsOfType(make_asset_type(AssetType::UserPersona)).size() == 1uz)
			Global::GetUserContent().MarkDefault(_assetId);

		Global::GetUserContent().UpdateAsset(_assetId, _character);
		Global::GetUserContent().GetAssets().SaveNow();
		return true;
	}

	void UserPersonaEditor::OnClose()
	{
		Global::GetUserContent().GetAssets().DiscardTemporaryAssets();
	}

	std::vector<EditorTabDescriptor> UserPersonaEditor::GetTabDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorTabDescriptor> {
			{
				Tab::General,
				"Identity",
				Icon::CHARACTER_EDIT_INFO,
				Icon::CHARACTER_EDIT_INFO_SMALL,
			},
		};
	}

	void UserPersonaEditor::OnPropertyChanged()
	{
		_pSaveButton->SetTheme(ButtonThemes::SaveButton);
	}

	void UserPersonaEditor::SaveChanges()
	{
		if (Save())
		{
			PushEvent(UserEvent::NavigateToUserPersonas);
			return;
		}
		
		auto& dlg = CreateDialogBox("An error occurred while saving.", DialogBoxType::Ok);
		dlg.Show();
	}
}