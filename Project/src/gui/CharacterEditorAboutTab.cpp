#include <pch.h>
#include "gui/CharacterEditorAboutTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "data/Character.h"
#include "util/StringUtils.h"

using namespace fig::data;

namespace fig::gui
{
	CharacterEditorAboutTab::CharacterEditorAboutTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool CharacterEditorAboutTab::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		_pCharacter = args.pCharacter;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "About character");

		// Notes
		CreateLabel(this, pSizer, "Character description");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pCharacter->about), 4);
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(8);
		pDescription->SetPlaceholder("Enter a brief character description here (for the end-user)");

		// Tags
		CreateLabel(this, pSizer, "Tags");
		_pTags = CreateTextBox(this, pSizer);
		_pTags->SetMaxWidth(620);
		_pTags->SetPlaceholder("Enter tags separated by commas");
		_pTags->SetText(encode_csv(_pCharacter->GetTags()));

		// Author
		static fig::string temp_author;
		CreateLabel(this, pSizer, "Creator");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pCharacter->creator))
			->SetMaxWidth(300);

		// Version
		static fig::string temp_version;
		CreateLabel(this, pSizer, "Version");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pCharacter->version))
			->SetMaxWidth(120);

		return true;
	}

	void CharacterEditorAboutTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}
	
	bool CharacterEditorAboutTab::Save()
	{
		auto tags = decode_csv(_pTags->GetText())
			| std::ranges::to<std::unordered_set>()
			| std::ranges::to<std::vector>();
		_pCharacter->SetTags(tags);
		return true;
	}
}