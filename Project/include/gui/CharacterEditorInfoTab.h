#pragma once

#include "gui/EditorTab.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class CharacterEditorInfoTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorInfoTab(ControlPtr pParent);
		
		bool Initialize(CharacterEditorArgs args) override;
		void ShutDown() noexcept {};
	
	protected:
		void OnAfterLayout();

		fig::observer_ptr<fig::data::Character> _pCharacter {};
	};
}