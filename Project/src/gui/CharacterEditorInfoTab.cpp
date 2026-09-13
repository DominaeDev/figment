#include <pch.h>
#include "gui/CharacterEditorInfoTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/CharacterAttributeWidget.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/Menu.h"
#include "data/Character.h"

using namespace fig::data;

namespace fig::gui
{
	CharacterEditorInfoTab::CharacterEditorInfoTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool CharacterEditorInfoTab::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		_pCharacter = args.pCharacter;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Basic information");

		// Name(s)
		auto pNameSizer = new HorizontalSizer();
		auto pNameColumn1 = new VerticalSizer();
		auto pNameColumn2 = new VerticalSizer();
		pNameSizer->Add(pNameColumn1, 0, SizerFlag::FixedSize, 320);
		pNameSizer->Add(pNameColumn2, -1);
		CreateLabel(this, pNameColumn1, "First name");
		CreateTextBox(this, pNameColumn1, ValueBinding<fig::string>(&_pCharacter->name.first))
			->SetMaxWidth(300);

		CreateLabel(this, pNameColumn2, "Last name");
		CreateTextBox(this, pNameColumn2, ValueBinding<fig::string>(&_pCharacter->name.last))
			->SetMaxWidth(300);

		CreateLabel(this, pNameColumn1, "Nickname");
		CreateTextBox(this, pNameColumn1, ValueBinding<fig::string>(&_pCharacter->name.nickname))
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
		CreateComboBox(this, pGenderColumn1, genders, ValueBinding<Gender>(&_pCharacter->gender))
			->SetMaxWidth(300);

		CreateLabel(this, pGenderColumn2, "Pronouns");
		CreateDropList(this, pGenderColumn2, pronouns, ValueBinding<Pronouns>(&_pCharacter->pronouns))
			->SetMaxWidth(180);

		pSizer->Add(pGenderSizer, 0, SizerFlag::FixedSize, 63);

		// Age
		CreateLabel(this, pSizer, "Age");
		auto pAge = CreateTextBox(this, pSizer);
		pAge->SetText(_pCharacter->GetAttribute("age").value_or(""));
		pAge->SetTextChangedCallback([&](fig::string_view text) mutable { _pCharacter->SetAttribute("age", "Age", text); });
		pAge->SetMaxWidth(120);

		CreateHorizontalLine(this, pSizer);

		// Attributes
		CreateHeader(this, pSizer, "Attributes");

		bool bHasAttributes = false;
		for (auto& kvp : _pCharacter->GetAttributes())
		{
			auto& key = kvp.first;
			auto& attribute = kvp.second;
			
			if (key == "age")
				continue; // Skip

			pSizer->AddSpacer(8);
			auto pAttribute = CreateControl<CharacterAttributeWidget>(attribute);
			pSizer->Add(pAttribute, 0, SizerFlag::Expand);
			bHasAttributes = true;
		}
		if (bHasAttributes)
			pSizer->AddSpacer(12);

		auto pAddAttributeButton = CreateControl<ButtonWithLabel>("Add attribute");
		pAddAttributeButton->SetHeight(35);
		pAddAttributeButton->SetDelegate([this] { ShowAttributesMenu(); });
		pSizer->Add(pAddAttributeButton, 0);
		
		CreateHorizontalLine(this, pSizer);

		// Traits
		CreateHeader(this, pSizer, "Traits");
		CreateLabel(this, pSizer, "Positive (0)");
		CreateLabel(this, pSizer, "Negative (0)");
		CreateLabel(this, pSizer, "Neutral (0)");


		return true;
	}

	void CharacterEditorInfoTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}
		
	void CharacterEditorInfoTab::ShowAttributesMenu()
	{
		auto& menu = CreateMenu();
		menu.AddItem("New attribute");
		menu.AddSeparator();

		auto& generalMenu = menu.AddItem("General");
		generalMenu.AddItem("Aliases");
		generalMenu.AddItem("Occupation");
		generalMenu.AddItem("Background");
		generalMenu.AddItem("Reputation");

		auto& appearanceMenu = menu.AddItem("Appearance");
		appearanceMenu.AddItem("Appearance (summary)");
		appearanceMenu.AddItem("Clothes");
		appearanceMenu.AddItem("Hair");
		appearanceMenu.AddItem("Eyes");
		appearanceMenu.AddItem("Body");
		appearanceMenu.AddItem("Skin");
		appearanceMenu.AddItem("Speech");
		appearanceMenu.AddItem("Height");
		appearanceMenu.AddItem("Weight");

		auto& mentalMenu = menu.AddItem("Psychology");
		mentalMenu.AddItem("Personality");
		mentalMenu.AddItem("Interests");
		mentalMenu.AddItem("Hobbies");
		mentalMenu.AddItem("Likes");
		mentalMenu.AddItem("Dislikes");
		mentalMenu.AddItem("Motivation/Goals");
		mentalMenu.AddItem("Opinins/Beliefs");
		
		auto& miscMenu = menu.AddItem("Miscellaneous");
		miscMenu.AddItem("Blood type");
		miscMenu.AddItem("Astrological sign");

		menu.Show();
	}
}