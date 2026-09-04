#include <pch.h>
#include "gui/CharacterEditor.h"
#include "gui/EditorFields.h"
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
	CharacterEditor::CharacterEditor(const fig::uuid& characterId) : Editor(nullptr)
	{
		if (auto try_character = Global::GetUserContent().Get<Character>(characterId))
			_value = fig::data::Character { *try_character };
	}

	void CharacterEditor::Initialize() noexcept
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
		CreateLabel(this, pSizer, "Description");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_value.description), 4)
			->SetMaxWidth(620);
		CreateHint(this, pSizer, "A brief summary of the character for the user.");

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

	void CharacterEditor::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	fig::string CharacterEditor::GetTitle() const noexcept
	{
		return "Editing character";
	}

	EditorFields CharacterEditor::GetFields() noexcept
	{
		EditorFields fields;

		fields.push_back(EditorHeader("Character name"));
		fields.push_back(std::make_shared<EditorTextField<fig::string>>(&_value.name.first, "First name", 250));
		fields.push_back(std::make_shared<EditorTextField<fig::string>>(&_value.name.last, "Last name", 250));
		fields.push_back(std::make_shared<EditorTextField<fig::string>>(&_value.name.nickname, "Nickname", 250));

		return fields;
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
		return true;
	}
}