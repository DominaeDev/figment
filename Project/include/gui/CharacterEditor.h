#pragma once

#include "gui/Editor.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class CharacterEditor : public Editor
	{
	public:
		CharacterEditor(const fig::uuid& characterId);
		
		void Initialize() noexcept;
		void ShutDown() noexcept {};

		fig::string GetTitle() const noexcept override;
		EditorFields GetFields() noexcept override;
		bool SaveChanges() noexcept override;

	private:
		fig::data::Character _value {};
	};
}