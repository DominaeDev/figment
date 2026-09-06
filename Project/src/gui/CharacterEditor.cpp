#include <pch.h>
#include "gui/CharacterEditor.h"
#include "gui/CharacterEditorInfoPage.h"
#include "gui/CharacterEditorVoicePage.h"
#include "gui/CharacterEditorAboutPage.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	CharacterEditor::CharacterEditor(ControlPtr pParent, const fig::uuid& assetId) : Editor(pParent)
	{
		CreatePage<CharacterEditorInfoPage>(assetId);
		CreatePage<CharacterEditorVoicePage>(assetId);
		CreatePage<CharacterEditorAboutPage>(assetId);
	}

	fig::string CharacterEditor::GetTitle() const noexcept
	{
		return "Editing character";
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
		for (auto& page : _pages)
			page->OnSave();
		return true;
	}

	void CharacterEditor::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}
}