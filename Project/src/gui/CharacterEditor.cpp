#include <pch.h>
#include "gui/CharacterEditor.h"
#include "gui/CharacterEditorGeneralTab.h"
#include "gui/CharacterEditorTraitsTab.h"
#include "gui/CharacterEditorImagesTab.h"
#include "gui/CharacterEditorVoiceTab.h"
#include "gui/CharacterEditorRulesTab.h"
#include "gui/CharacterEditorAboutTab.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	constexpr enum Tab : int32_t
	{
		General = 0,
		Traits,
		Images,
		Voice,
		Rules,
		About,
	};


	CharacterEditor::CharacterEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreateTab<CharacterEditorGeneralTab>();
		CreateTab<CharacterEditorTraitsTab>();
		CreateTab<CharacterEditorImagesTab>();
		CreateTab<CharacterEditorVoiceTab>();
		CreateTab<CharacterEditorRulesTab>();
		CreateTab<CharacterEditorAboutTab>();
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

			for (auto tab : _tabs | std::views::transform([](auto&& t) { return dynamic_cast<EditorTab<CharacterEditorArgs>*>(t.get()); }))
			{
				if (not (tab and tab->Initialize(args)))
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

	bool CharacterEditor::Save() noexcept
	{
		if (_assetId.empty())
			return false;

		bool bOk = true;
		for (auto& tab : _tabs)
		{
			if (auto result = tab->OnSave(); not result.has_value())
			{
				bOk = false;
				break;
			}
		}

		if (not bOk)
		{
			LogLn("Error occurred while saving character."); //! @todo: User facing error
			return false; // Error
		}
		
		Global::GetUserContent().UpdateAsset(_assetId, _character);
		Global::GetUserContent().GetAssets().SaveNow();

		_pSaveButton->SetTheme(DefaultButtonStyle);
		auto pBorder = _pSaveButton->GetBorderRenderer();
		pBorder->SetColor(Color::Border);
		return true;
	}

	void CharacterEditor::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	std::vector<EditorTabDescriptor> CharacterEditor::GetTabDescriptors() const
	{
		static size_t NotImpl = (size_t)(-1);
		return std::vector<EditorTabDescriptor> {
			{
				Tab::General,
				"General",
				Resource::ICON_CHARACTER_EDIT_INFO,
				Resource::ICON_CHARACTER_EDIT_INFO_SMALL,
			},
			{
				Tab::Traits,
				"Traits",
				Resource::ICON_CHARACTER_EDIT_TRAITS,
				Resource::ICON_CHARACTER_EDIT_TRAITS_SMALL,
			},
			{
				Tab::Images,
				"Images",
				Resource::ICON_CHARACTER_EDIT_IMAGES,
				Resource::ICON_CHARACTER_EDIT_IMAGES_SMALL,
			},
			{
				Tab::Voice,
				"Voice",
				Resource::ICON_CHARACTER_EDIT_VOICE,
				Resource::ICON_CHARACTER_EDIT_VOICE_SMALL,
			},
			{
				NotImpl,
				"Story",
				Resource::ICON_CHARACTER_EDIT_STORY,
				Resource::ICON_CHARACTER_EDIT_STORY_SMALL,
			},
			{
				NotImpl,
				"Memories",
				Resource::ICON_CHARACTER_EDIT_MEMORIES,
				Resource::ICON_CHARACTER_EDIT_MEMORIES_SMALL,
			},
			{
				Tab::Rules,
				"Rules",
				Resource::ICON_CHARACTER_EDIT_RULES,
				Resource::ICON_CHARACTER_EDIT_RULES_SMALL,
			},
			{
				NotImpl,
				"Concepts",
				Resource::ICON_CHARACTER_EDIT_CONCEPTS,
				Resource::ICON_CHARACTER_EDIT_CONCEPTS_SMALL,
			},
			{
				Tab::About,
				"About",
				Resource::ICON_CHARACTER_EDIT_ABOUT,
				Resource::ICON_CHARACTER_EDIT_ABOUT_SMALL,
			},
		};
	}

	void CharacterEditor::OnPropertyChanged()
	{
		_pSaveButton->SetTheme(SaveButtonStyle);
	}
}