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
	class CharacterAttributeWidget;
	class ButtonWithLabel;

	class CharacterEditorRulesTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorRulesTab(ControlPtr pParent);

		bool Initialize(CharacterEditorArgs args) override;
		SaveResult OnSave() noexcept override;

	private:
		fig::observer_ptr<CharacterAttributeWidget> AddRule();
		fig::observer_ptr<CharacterAttributeWidget> AddRule(const fig::data::CharacterRule& rule);
		fig::observer_ptr<CharacterAttributeWidget> AppendControl(fig::string_view value, size_t index);
		void RemoveRule(size_t index);
		void OnMoveRule(size_t index, int32_t dir, bool bMaxDistance);
		void RefreshRuleLabels();

	private:
		void OnRuleMenu(size_t index);
		void OnAfterLayout();
		void OnCopyRule(size_t index);
		void OnPasteRule(size_t index);

		fig::observer_ptr<fig::data::Character> _pCharacter {};

		struct RuleItem
		{
			size_t index;
			fig::observer_ptr<CharacterAttributeWidget> pControl;
		};

		std::vector<RuleItem> _items;
		static size_t _nextRuleIndex;
		SizerPtr _pRuleSizer {};
		fig::observer_ptr<ButtonWithLabel> _pAddButton;
	};
}