#pragma once

#include "gui/EditorTab.h"
#include "gui/CharacterEditorArgs.h"
#include "data/CharacterTraitInfo.h"
#include "io/XmlData.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class ToggleWithLabel;

	class CharacterEditorTraitsTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorTraitsTab(ControlPtr pParent);

		bool Initialize(CharacterEditorArgs args) override;

	private:
		fig::observer_ptr<ToggleWithLabel> CreateTrait(SizerPtr pSizer, fig::handle traitId, fig::string_view label);
		void OnToggledTrait(const fig::handle& traitId, bool bOn);
		void RefreshToggleGroupLabels();

		fig::data::CharacterTraitInfoDatabase _traitsInfo;

	private:
		void OnAfterLayout();

		fig::observer_ptr<fig::data::Character> _pCharacter {};

		std::map<fig::handle, fig::observer_ptr<ToggleWithLabel>> _traitToggles;
		fig::observer_ptr<StaticText> _traitsLabel;
		std::map<fig::string, fig::observer_ptr<StaticText>> _traitGroupLabels;
	};
}