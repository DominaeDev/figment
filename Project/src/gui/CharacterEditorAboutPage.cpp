#include <pch.h>
#include "gui/CharacterEditorAboutPage.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "data/Character.h"

using namespace fig::data;

namespace fig::gui
{
	CharacterEditorAboutPage::CharacterEditorAboutPage(ControlPtr pParent, const fig::uuid& characterId) : EditorPage(pParent)
	{
		if (auto try_character = Global::GetUserContent().Get<Character>(characterId))
			_value = fig::data::Character { *try_character };

		Initialize();
	}

	void CharacterEditorAboutPage::Initialize() noexcept
	{
		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "About character");

		// Author
		static fig::string temp_author;
		CreateLabel(this, pSizer, "Creator");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_value.creator))
			->SetMaxWidth(300);

		// Notes
		CreateLabel(this, pSizer, "Creator's notes");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_value.about), 4);
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(8);

		// Tags
		CreateLabel(this, pSizer, "Tags");
		CreateTextBox(this, pSizer)
			->SetMaxWidth(620);

		// Version
		static fig::string temp_version;
		CreateLabel(this, pSizer, "Version");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_value.version))
			->SetMaxWidth(120);
	}

	void CharacterEditorAboutPage::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	fig::string CharacterEditorAboutPage::GetName() const noexcept
	{
		return "About";
	}
		
}