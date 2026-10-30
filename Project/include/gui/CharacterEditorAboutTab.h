#pragma once

#include "gui/EditorTab.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::gui
{
	class CharacterEditorAboutTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorAboutTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;

	protected:
		bool OnInit(CharacterEditorArgs args) override;

	private:
		fig::observer_ptr<fig::data::Character> _pCharacter {};
		fig::observer_ptr<TextBox> _pTags {};
	};
}