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
	
		void PopulateTopBar(ControlPtr pParent);
		bool Save() noexcept;

	protected:
		void OnAfterLayout();

	private:
		fig::data::Character _value {};
		fig::observer_ptr<Control> _pSaveButton;
	};
}