#pragma once

#include "gui/EditorPage.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class CharacterEditorInfoPage : public EditorPage<CharacterEditorArgs>
	{
	public:
		CharacterEditorInfoPage(ControlPtr pParent);
		
		bool Initialize(CharacterEditorArgs args) override;
		void ShutDown() noexcept {};
	
	protected:
		void OnAfterLayout();

		fig::observer_ptr<fig::data::Character> _pCharacter {};
	};
}