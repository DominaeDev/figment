#include <pch.h>
#include "gui/CharacterEditorInfoPage.h"
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
	CharacterEditorInfoPage::CharacterEditorInfoPage(ControlPtr pParent, const fig::uuid& characterId) : EditorPage(pParent)
	{
		if (auto try_character = Global::GetUserContent().Get<Character>(characterId))
			_value = fig::data::Character { *try_character };

		Initialize();
	}

	void CharacterEditorInfoPage::Initialize() noexcept
	{
		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Character details");

		// Name(s)
		auto pNameSizer = new HorizontalSizer();
		auto pNameColumn1 = new VerticalSizer();
		auto pNameColumn2 = new VerticalSizer();
		pNameSizer->Add(pNameColumn1, 0, SizerFlag::FixedSize, 320);
		pNameSizer->Add(pNameColumn2, -1);
		CreateLabel(this, pNameColumn1, "First name");
		CreateTextBox(this, pNameColumn1, ValueBinding<fig::string>(&_value.name.first))
			->SetMaxWidth(300);

		CreateLabel(this, pNameColumn2, "Last name");
		CreateTextBox(this, pNameColumn2, ValueBinding<fig::string>(&_value.name.last))
			->SetMaxWidth(300);

		CreateLabel(this, pNameColumn1, "Nickname");
		CreateTextBox(this, pNameColumn1, ValueBinding<fig::string>(&_value.name.nickname))
			->SetMaxWidth(300);

		pSizer->Add(pNameSizer, 0, SizerFlag::FixedSize, 126);

		// Gender / Pronouns
		auto pGenderSizer = new HorizontalSizer();
		auto pGenderColumn1 = new VerticalSizer();
		auto pGenderColumn2 = new VerticalSizer();
		pGenderSizer->Add(pGenderColumn1, 0, SizerFlag::FixedSize, 320);
		pGenderSizer->Add(pGenderColumn2, -1);

		std::vector<fig::string> genders { "Male", "Female", "Non-binary" };
		std::vector<fig::string> pronouns { "Auto", "He/Him", "She/Her", "They/Them", "It/It" };
		CreateLabel(this, pGenderColumn1, "Gender");
		CreateComboBox(this, pGenderColumn1, genders, ValueBinding<Gender>(&_value.gender))
			->SetMaxWidth(300);

		CreateLabel(this, pGenderColumn2, "Pronouns");
		CreateDropList(this, pGenderColumn2, pronouns, ValueBinding<Pronouns>(&_value.pronouns))
			->SetMaxWidth(180);

		pSizer->Add(pGenderSizer, 0, SizerFlag::FixedSize, 63);

		// Age
		CreateLabel(this, pSizer, "Age");
		auto pAge = CreateControl<TextBox>();
		pAge->SetText(_value.GetAttribute("age").value_or(""));
		pAge->SetTextChangedCallback([&](fig::string_view text) mutable { _value.SetAttribute("age", "Age", text); });
		pAge->SetMaxWidth(100);
		pSizer->Add(pAge, 0, SizerFlag::Expand, 0);

		// Description
		CreateLabel(this, pSizer, "Notes");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_value.description), 4);
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(8);

		CreateHint(this, pSizer, "The notes do not affect the character's behavior.");

		// ----
		auto pLine = CreateControl<HorizontalLine>();
		pLine->SetMaxWidth(620);
		pSizer->AddSpacer(6);
		pSizer->Add(pLine, 0, SizerFlag::Expand);

		CreateHeader(this, pSizer, "Attributes");

		// Buttons
		auto pAddAttributeButton = CreateControl<ButtonWithLabel>("Add attribute");
		pAddAttributeButton->SetHeight(35);
//		pAddAttributeButton->SetDelegate([] { });
		
		pSizer->Add(pAddAttributeButton, 0);


/*		auto genderBinding = ValueBinding<Gender>(&_value.gender);
		auto pGender = CreateControl<ComboBox>();
		pGender->AddItems(std::vector<fig::string> { "Male", "Female", });
		pGender->SetMaxWidth(300);
		pGender->SetTextChangedCallback([genderBinding](const fig::string& text) mutable { genderBinding.Set(text); });
		pSizer->Add(pGender, 0, SizerFlag::Expand, 0); */

//		CreateLabel(this, pSizer, "Next item");
//		CreateTextBox<fig::string>(this, pSizer, ValueBinding<fig::string>(&_value.name.last))
//			->SetMaxWidth(250);
	}

	void CharacterEditorInfoPage::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	fig::string CharacterEditorInfoPage::GetName() const noexcept
	{
		return "Attributes";
	}
		
}