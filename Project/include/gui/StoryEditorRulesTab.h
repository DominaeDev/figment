#pragma once

#include "gui/EditorTab.h"
#include "gui/StoryEditorArgs.h"
#include "io/XmlData.h"

namespace fig::data
{
	class Scenario;
	using ScenarioRule = fig::string;
}

namespace fig::gui
{
	class CharacterAttributeWidget;
	class ButtonWithLabel;

	class StoryEditorRulesTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorRulesTab(ControlPtr pParent);

		bool Initialize(StoryEditorArgs args) override;
		SaveResult OnSave() noexcept override;

	private:
		fig::observer_ptr<CharacterAttributeWidget> AddRule();
		fig::observer_ptr<CharacterAttributeWidget> AddRule(const fig::data::ScenarioRule& rule);
		fig::observer_ptr<CharacterAttributeWidget> AppendControl(fig::string_view value, size_t index);
		void RemoveRule(size_t index);
		void OnMoveRule(size_t index, int32_t dir, bool bMaxDistance);
		void RefreshRuleLabels();

	private:
		void OnRuleMenu(size_t index);
		void OnCopyRule(size_t index);
		void OnPasteRule(size_t index);

		fig::observer_ptr<fig::data::Scenario> _pStory {};

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