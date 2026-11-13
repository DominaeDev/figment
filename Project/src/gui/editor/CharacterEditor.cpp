#include <pch.h>
#include "gui/editor/CharacterEditor.h"
#include "gui/editor/CharacterEditorGeneralTab.h"
#include "gui/editor/CharacterEditorTraitsTab.h"
#include "gui/editor/CharacterEditorImagesTab.h"
#include "gui/editor/CharacterEditorVoiceTab.h"
#include "gui/editor/CharacterEditorDialogueTab.h"
#include "gui/editor/CharacterEditorRulesTab.h"
#include "gui/editor/CharacterEditorAboutTab.h"
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
		Traits,
		Images,
		Voice,
		Dialogue,
		Rules,
		About,
	};

	CharacterEditor::CharacterEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreateTab<CharacterEditorGeneralTab>();
		CreateTab<CharacterEditorTraitsTab>();
		CreateTab<CharacterEditorImagesTab>();
		CreateTab<CharacterEditorVoiceTab>();
		CreateTab<CharacterEditorDialogueTab>();
		CreateTab<CharacterEditorRulesTab>();
		CreateTab<CharacterEditorAboutTab>();
	}

	fig::string CharacterEditor::GetTitle() const noexcept
	{
		return "Editing character";
	}
	
	bool CharacterEditor::NewCharacter(const fig::uuid& folderId) noexcept
	{
		auto& asset = Global::GetUserContent().GetAssets().CreateTemporaryAsset(make_asset_type(AssetType::Character, DataFormat::TextXml), fig::uuid {}, folderId);

		_assetId = asset.id;
		_character = {};
		_character.name.first = "Unnamed";

		CharacterEditorArgs args
		{
			.assetId = _assetId,
			.pCharacter = &_character,
		};

		InitTabs(args);
		return true;
	}

	bool CharacterEditor::EditCharacter(const fig::uuid& assetId) noexcept
	{
		if (auto try_character = Global::GetUserContent().Get<fig::data::Character>(assetId))
		{
			_assetId = assetId;
			_character = fig::data::Character { *try_character };
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

	void CharacterEditor::PopulateTopBar(ControlPtr pParent)
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

	bool CharacterEditor::Save() noexcept
	{
		if (_assetId.empty())
			return false;

		bool bOk = SaveTabs();

		_character.RebuildSearchIndex();

		if (not bOk)
		{
			LogLn("Error occurred while saving character."); //! @todo: User facing error
			return false; // Error
		}
		
		Global::GetUserContent().UpdateAsset(_assetId, _character);
		Global::GetUserContent().GetAssets().SaveNow();
		return true;
	}

	void CharacterEditor::OnClose()
	{
		Global::GetUserContent().GetAssets().DiscardTemporaryAssets();
	}

	std::vector<EditorTabDescriptor> CharacterEditor::GetTabDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorTabDescriptor> {
			{
				Tab::General,
				"General",
				Icon::CHARACTER_EDIT_INFO,
				Icon::CHARACTER_EDIT_INFO_SMALL,
			},
			{
				Tab::Traits,
				"Traits",
				Icon::CHARACTER_EDIT_TRAITS,
				Icon::CHARACTER_EDIT_TRAITS_SMALL,
			},
			{
				Tab::Images,
				"Images",
				Icon::CHARACTER_EDIT_IMAGES,
				Icon::CHARACTER_EDIT_IMAGES_SMALL,
			},
			{
				Tab::Voice,
				"Voice",
				Icon::CHARACTER_EDIT_VOICE,
				Icon::CHARACTER_EDIT_VOICE_SMALL,
			},
			{
				Tab::Dialogue,
				"Example chat",
				Icon::MENU_CHATS,
				Icon::MENU_CHATS_SMALL,
			},
			{
				NotImpl,
				"Memories",
				Icon::CHARACTER_EDIT_MEMORIES,
				Icon::CHARACTER_EDIT_MEMORIES_SMALL,
			},
			{
				Tab::Rules,
				"Rules",
				Icon::CHARACTER_EDIT_RULES,
				Icon::CHARACTER_EDIT_RULES_SMALL,
			},
			{
				NotImpl,
				"Concepts",
				Icon::CHARACTER_EDIT_CONCEPTS,
				Icon::CHARACTER_EDIT_CONCEPTS_SMALL,
			},
			{
				Tab::About,
				"About",
				Icon::CHARACTER_EDIT_ABOUT,
				Icon::CHARACTER_EDIT_ABOUT_SMALL,
			},
		};
	}

	void CharacterEditor::OnPropertyChanged()
	{
		_pSaveButton->SetTheme(ButtonThemes::SaveButton);
	}

	void CharacterEditor::SaveChanges()
	{
		if (Save())
		{
			PushEvent(UserEvent::NavigateToHome);
			return;
		}
		
		auto& dlg = CreateDialogBox("An error occurred while saving.", DialogBoxType::Ok);
		dlg.Show();
	}
}