#include <pch.h>
#include "gui/CharacterEditor.h"
#include "gui/EditorFields.h"
#include "gui/SimpleTextBox.h"
#include "data/Character.h"

using namespace fig::data;

namespace fig::gui
{
	CharacterEditor::CharacterEditor(const fig::uuid& characterId) : Editor(nullptr)
	{
		SetBackgroundColor(Color::Debug);

		if (auto try_character = Global::GetUserContent().Get<Character>(characterId))
			_value = fig::data::Character { *try_character };
	}

	void CharacterEditor::Initialize() noexcept
	{
		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Character name");

		auto pNameSizer = new HorizontalSizer();
		auto pColumn1 = new VerticalSizer();
		auto pColumn2 = new VerticalSizer();

		pNameSizer->Add(pColumn1, 0, SizerFlag::FixedSize, 340);
		pNameSizer->Add(pColumn2, -1);

		CreateLabel(this, pColumn1, "First name");
		CreateTextBox<fig::string>(this, pColumn1, ValueBinding<fig::string>(&_value.name.first))
			->SetMaxWidth(251);

		CreateLabel(this, pColumn1, "Nickname");
		CreateTextBox<fig::string>(this, pColumn1, ValueBinding<fig::string>(&_value.name.nickname))
			->SetMaxWidth(252);

		CreateLabel(this, pColumn2, "Last name");
		CreateTextBox<fig::string>(this, pColumn2, ValueBinding<fig::string>(&_value.name.last))
			->SetMaxWidth(253);

		pSizer->Add(pNameSizer, 0, SizerFlag::FixedSize, 160);

//		CreateLabel(this, pSizer, "Next item");
//		CreateTextBox<fig::string>(this, pSizer, ValueBinding<fig::string>(&_value.name.last))
//			->SetMaxWidth(250);

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

}