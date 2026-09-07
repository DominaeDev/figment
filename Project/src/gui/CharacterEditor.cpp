#include <pch.h>
#include "gui/CharacterEditor.h"
#include "gui/CharacterEditorInfoPage.h"
#include "gui/CharacterEditorVoicePage.h"
#include "gui/CharacterEditorAboutPage.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	CharacterEditor::CharacterEditor(ControlPtr pParent) : Editor(pParent)
	{
		CreatePage<CharacterEditorInfoPage>();
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

		auto pDiscardButton = pParent->CreateControl<ButtonWithLabelAndIcon>("Discard", Resource::ICON_DELETE);
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
		
		if (bOk and Global::GetUserContent().UpdateAsset(_assetId, _character))
			return true;

		LogLn("Error occurred while saving character."); //! @todo: User facing error
		return false;
	}

	void CharacterEditor::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}
}