#pragma once

#include "gui/EditorPage.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class CharacterEditorInfoPage : public EditorPage
	{
	public:
		CharacterEditorInfoPage(ControlPtr pParent, const fig::uuid& characterId);
		
		void Initialize() noexcept;
		void ShutDown() noexcept {};

		fig::string GetName() const noexcept;
	
	protected:
		void OnAfterLayout();

	private:
		fig::data::Character _value {};
	};
}