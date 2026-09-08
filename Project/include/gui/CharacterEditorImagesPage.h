#pragma once

#include "gui/EditorPage.h"
#include "gui/CharacterEditorArgs.h"

namespace fig::gui
{
	class CharacterEditorImagesPage : public EditorPage<CharacterEditorArgs>
	{
	public:
		CharacterEditorImagesPage(ControlPtr pParent);

		bool Initialize(CharacterEditorArgs args) override;
		void ShutDown() noexcept {};
		bool Save() override;

	protected:
		void OnAfterLayout();

	private:
		fig::uuid _characterId;
	};
}