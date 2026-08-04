#pragma once

#include "IEditor.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class CharacterEditor : public IEditor
	{
	public:
		CharacterEditor(const fig::uuid& characterId);
		
		fig::string GetTitle() const noexcept override;
		EditorFields GetFields() noexcept override;
		bool SaveChanges() noexcept override;

	private:
		fig::data::Character _value {};
	};
}