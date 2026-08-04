#include <pch.h>
#include "gui/CharacterEditor.h"
#include "gui/EditorFields.h"
#include "data/Character.h"

using namespace fig::data;

namespace fig::gui
{
	CharacterEditor::CharacterEditor(const fig::uuid& characterId)
	{
		if (auto try_character = Global::GetUserContent().Get<Character>(characterId))
			_value = fig::data::Character { *try_character };
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
		fields.push_back(EditorHint("(Supercedes the first name as the character's spoken name)"));

		return fields;
	}

	bool CharacterEditor::SaveChanges() noexcept
	{
		return false;
	}
}