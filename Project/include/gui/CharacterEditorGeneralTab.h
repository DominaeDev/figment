#pragma once

#include "gui/EditorTab.h"
#include "gui/CharacterEditorArgs.h"
#include "data/CharacterAttributeInfo.h"
#include "data/CharacterTraitInfo.h"
#include "io/XmlData.h"

namespace fig::data
{
	class Character;
}

namespace fig::gui
{
	class CharacterAttributeWidget;
	class TextBox;
	class ToggleWithLabel;

	class CharacterEditorGeneralTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorGeneralTab(ControlPtr pParent);
		
		bool Initialize(CharacterEditorArgs args) override;
		SaveResult OnSave() noexcept override;
		void ShutDown() noexcept {};

	private:
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute();
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute(const fig::data::CharacterAttribute& attribute);
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute(const fig::data::CharacterAttributeInfo& info);
		fig::observer_ptr<CharacterAttributeWidget> AppendAttributeControl(fig::data::CharacterAttribute& attribute, size_t index, const fig::string_list& options, fig::string_view placeholder);
		void RenameAttribute(size_t index);
		void OnRenamedAttribute(size_t index, fig::string_view name);
		void RemoveAttribute(size_t index);
		void OnMoveAttribute(size_t index, int32_t dir, bool bMaxDistance);

		fig::observer_ptr<ToggleWithLabel> CreateTrait(SizerPtr pSizer, fig::handle traitId, fig::string_view label);
		void OnToggledTrait(const fig::handle& traitId, bool bOn);
		void RefreshToggleGroupLabels();

		fig::data::CharacterAttributeInfoDatabase _attributesInfo;
		fig::data::CharacterTraitInfoDatabase _traitsInfo;
	private:
		void ShowAttributesMenu();
		void OnAttributeSettingsMenu(size_t attributeIndex);
		void OnAfterLayout();

		fig::observer_ptr<fig::data::Character> _pCharacter {};

		struct AttributeItem
		{
			size_t index;
			fig::observer_ptr<CharacterAttributeWidget> pControl;
			fig::data::CharacterAttribute attribute;
		};

		std::vector<AttributeItem> _items;
		static size_t _nextAttributeIndex;

		fig::observer_ptr<TextBox> _pAge;
		SizerPtr _pAttributeSizer {};
		std::map<fig::string, fig::observer_ptr<StaticText>> _traitGroupLabels;
	};
}