#pragma once

#include "gui/EditorPage.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::gui
{
	class CharacterEditorAboutPage : public EditorPage<CharacterEditorArgs>
	{
	public:
		CharacterEditorAboutPage(ControlPtr pParent);

		bool Initialize(CharacterEditorArgs args) override;
		void ShutDown() noexcept {};

	protected:
		void OnAfterLayout();

	private:
		fig::observer_ptr<fig::data::Character> _pCharacter {};
	};
}