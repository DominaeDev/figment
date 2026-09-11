#include <pch.h>
#include "gui/CharacterEditor.h"
#include "gui/CharacterEditorInfoPage.h"
#include "gui/CharacterEditorImagesPage.h"
#include "gui/CharacterEditorVoicePage.h"
#include "gui/CharacterEditorAboutPage.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	CharacterEditor::CharacterEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreatePage<CharacterEditorInfoPage>();
		CreatePage<CharacterEditorImagesPage>();
		CreatePage<CharacterEditorVoicePage>();
		CreatePage<CharacterEditorAboutPage>();
	}

	fig::string CharacterEditor::GetTitle() const noexcept
	{
		return "Editing character";
	}

	bool CharacterEditor::Initialize(const fig::uuid& assetId) noexcept
	{
		if (auto try_character = Global::GetUserContent().Get<fig::data::Character>(assetId))
		{
			_assetId = assetId;
			_character = fig::data::Character { *try_character };
			CharacterEditorArgs args
			{
				.assetId = assetId,
				.pCharacter = &_character,
			};

			for (auto page : _pages | std::views::transform([](auto&& p) { return dynamic_cast<EditorPage<CharacterEditorArgs>*>(p.get()); }))
			{
				if (not (page and page->Initialize(args)))
					return false;
			}

			return true;
		}

		_assetId = {};
		return false;
	}

	void CharacterEditor::PopulateTopBar(ControlPtr pParent)
	{
		auto pSizer = pParent->GetSizer();

		auto pSaveButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Save", Resource::ICON_SAVE);
		pSaveButton->SetSize(110, 32);
		pSaveButton->SetDelegate([this] {
			if (Save())
				PushEvent(UserEvent::NavigateToHome);
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

	bool CharacterEditor::Save() noexcept
	{
		if (_assetId.empty())
			return false;

		bool bOk = true;
		for (auto& page : _pages)
			bOk &= page->Save();
		
		bOk &= Global::GetUserContent().UpdateAsset(_assetId, _character);

		if (bOk)
			Global::GetUserContent().GetAssets().SaveNow();
		else
			LogLn("Error occurred while saving character."); //! @todo: User facing error
		return bOk;
	}

	void CharacterEditor::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	std::vector<EditorPageDescriptor> CharacterEditor::GetPageDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorPageDescriptor> {
			{
				0,
				"General",
				Resource::ICON_CHARACTER_EDIT_INFO,
				Resource::ICON_CHARACTER_EDIT_INFO_SMALL,
			},
			{
				NotImpl,
				"Story",
				Resource::ICON_CHARACTER_EDIT_STORY,
				Resource::ICON_CHARACTER_EDIT_STORY_SMALL,
			},
			{
				1,
				"Images",
				Resource::ICON_CHARACTER_EDIT_IMAGES,
				Resource::ICON_CHARACTER_EDIT_IMAGES_SMALL,
			},
			{
				2,
				"Voice",
				Resource::ICON_CHARACTER_EDIT_VOICE,
				Resource::ICON_CHARACTER_EDIT_VOICE_SMALL,
			},
			{
				NotImpl,
				"Memories",
				Resource::ICON_CHARACTER_EDIT_MEMORIES,
				Resource::ICON_CHARACTER_EDIT_MEMORIES_SMALL,
			},
			{
				NotImpl,
				"Concepts",
				Resource::ICON_CHARACTER_EDIT_CONCEPTS,
				Resource::ICON_CHARACTER_EDIT_CONCEPTS_SMALL,
			},
			{
				3,
				"About",
				Resource::ICON_CHARACTER_EDIT_ABOUT,
				Resource::ICON_CHARACTER_EDIT_ABOUT_SMALL,
			},
		};
	}
}