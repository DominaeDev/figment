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
		
		SaveResult OnSave() noexcept override;

	protected:
		bool OnInit(CharacterEditorArgs args) override;

	private:
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute();
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute(const fig::data::CharacterAttribute& attribute);
		fig::observer_ptr<CharacterAttributeWidget> AddAttribute(const fig::data::CharacterAttributeInfo& info);
		fig::observer_ptr<CharacterAttributeWidget> AppendAttributeControl(fig::data::CharacterAttribute& attribute, size_t index, const fig::string_list& options, fig::string_view placeholder);
		void RenameAttribute(size_t index);
		void OnRenamedAttribute(size_t index, fig::string_view name);
		void OnRemoveAttribute(size_t index);
		void OnMoveAttribute(size_t index, int32_t dir, bool bMaxDistance);
		void OnCopyAttribute(size_t index);
		void OnPasteAttribute(size_t index);
		void OnChangedAttributeValueType(size_t index, fig::data::CharacterAttribute::ValueType valueType);
		void OnChangedAttributeVisibility(size_t index, fig::data::CharacterAttribute::Visibility visibility);
		void OnChangedAttributeFlags(size_t index, fig::data::CharacterAttribute::HintFlags flags);


		void ShowAttributesMenu();
		void OnAttributeSettingsMenu(size_t attributeIndex);

		fig::data::CharacterAttributeInfoDatabase _attributesInfo;
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
	};
}