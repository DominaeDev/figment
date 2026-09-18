#include <pch.h>
#include "gui/CharacterEditorRulesTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/ToggleWithLabel.h"
#include "gui/CharacterAttributeWidget.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "gui/Menu.h"
#include "gui/GridSizer.h"
#include "data/Character.h"
#include "io/FileUtility.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	size_t CharacterEditorRulesTab::_nextRuleIndex {};
	static constexpr size_t kMaxRuleCount = 15uz;

	CharacterEditorRulesTab::CharacterEditorRulesTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool CharacterEditorRulesTab::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		_pCharacter = args.pCharacter;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Rules");

		_pAddButton = CreateControl<ButtonWithLabel>("Add rule");

		_pRuleSizer = new VerticalSizer();
		pSizer->Add(_pRuleSizer.get(), 0, SizerFlag::Expand | SizerFlag::Bottom, 8);

		size_t ruleCount = 0uz;
		for (auto& rule : _pCharacter->GetRules())
		{
			AddRule(rule);
			++ruleCount;
		}
		for (; ruleCount < 5; ++ruleCount)
			AddRule();

		pSizer->AddSpacer(6);

		_pAddButton->SetHeight(35);
		_pAddButton->SetDelegate([this] { auto pRule = AddRule(); pRule->Focus(); SetDirty(); });
		pSizer->Add(_pAddButton, 0);

		CreateHint(this, pSizer, std::format("You can add up to {} rules.", kMaxRuleCount));

		return true;
	}

	void CharacterEditorRulesTab::OnAfterLayout()
	{
		ResizeToFit(false, true);
	}

	static bool IsShiftDown()
	{
		auto mod = SDL_GetModState();
		return (mod & SDL_KMOD_SHIFT) != 0 and (mod & SDL_KMOD_CTRL) == 0 and (mod & SDL_KMOD_ALT) == 0;
	}

	fig::observer_ptr<CharacterAttributeWidget> CharacterEditorRulesTab::AppendControl(fig::string_view value, size_t index)
	{
		auto pRule = CreateControl<CharacterAttributeWidget>("", value, CharacterAttribute::ValueType::ShortText);
		pRule->SetButtonDelegate([this, index] { OnRuleMenu(index); });
		pRule->SetValueChangedDelegate([this] (auto&& _) { SetDirty(); });
		pRule->SetMoveDelegate([this, index](auto&& dir) { OnMoveRule(index, dir, IsShiftDown()); });
		_pRuleSizer->Add(pRule, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
		return pRule;
	}

	fig::observer_ptr<CharacterAttributeWidget> CharacterEditorRulesTab::AddRule()
	{
		return AddRule("");
	}

	fig::observer_ptr<CharacterAttributeWidget> CharacterEditorRulesTab::AddRule(const fig::data::CharacterRule& rule)
	{
		auto item = RuleItem {
			.index = _nextRuleIndex++,
		};

		item.pControl = AppendControl(rule, item.index);
		_items.push_back(std::move(item));
		RefreshRuleLabels();
		return item.pControl;
	}

	void CharacterEditorRulesTab::OnRuleMenu(size_t ruleIndex)
	{
		RuleItem* pItem = nullptr;
		size_t index {};
		if (auto itFind = std::ranges::find(_items, ruleIndex, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			pItem = &(*itFind);
			index = std::distance(_items.begin(), itFind);
		}
		else
			return;

		auto pControl = pItem->pControl;

		auto& menu = CreateMenu();
		menu.AddItem("Move to top")
			.SetEnabled(index > 0uz)
			.SetDelegate([this, ruleIndex] { OnMoveRule(ruleIndex, -1, true); });
		menu.AddItem("Move up")
			.SetEnabled(index > 0uz)
			.SetDelegate([this, ruleIndex] { OnMoveRule(ruleIndex, -1, false); });
		menu.AddItem("Move down")
			.SetEnabled(index + 1uz < _items.size())
			.SetDelegate([this, ruleIndex] { OnMoveRule(ruleIndex, 1, false); });
		menu.AddItem("Move to bottom")
			.SetEnabled(index + 1uz < _items.size())
			.SetDelegate([this, ruleIndex] { OnMoveRule(ruleIndex, 1, true); });
		menu.AddSeparator();
		menu.AddItem("Copy")
			.SetDelegate([this, ruleIndex] { OnCopyRule(ruleIndex); });
		menu.AddItem("Paste")
			.SetEnabled(SDL_HasClipboardText())
			.SetDelegate([this, ruleIndex] { OnPasteRule(ruleIndex); });
		menu.AddSeparator();
		menu.AddItem("Remove", Resource::ICON_DELETE)
			.SetDelegate([this, ruleIndex] { RemoveRule(ruleIndex); });

		menu.Show();
	}

	void CharacterEditorRulesTab::RemoveRule(size_t index)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			auto& item = *itFind;
			_pRuleSizer->Remove(item.pControl);
			DestroyChild(item.pControl);
			_items.erase(itFind);
			RefreshRuleLabels();
			SetDirty();
		}
	}

	void CharacterEditorRulesTab::OnMoveRule(size_t ruleIndex, int32_t dir, bool bMaxDistance)
	{
		if (auto itFind = std::ranges::find(_items, ruleIndex, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			size_t index = std::distance(std::ranges::begin(_items), itFind);
			if (dir < 0 and bMaxDistance) // Move to top
			{
				if (index == 0uz)
					return;
				RuleItem item = *itFind;
				_items.erase(itFind);
				_items.insert(_items.cbegin(), item);
			}
			else if (dir < 0) // Move up
			{
				if (index == 0uz)
					return;
				std::swap(_items[index - 1], _items[index]);
			}
			if (dir > 0 and bMaxDistance) // Move to bottom
			{
				if (index + 1 >= _items.size())
					return;

				RuleItem item = *itFind;
				_items.erase(itFind);
				_items.insert(_items.cbegin() + _items.size(), item);
			}
			else if (dir > 0) // Move down
			{
				if (index + 1 >= _items.size())
					return;
				std::swap(_items[index], _items[index + 1]);
			}

			_pRuleSizer->RemoveAll();
			for (auto& item : _items)
				_pRuleSizer->Add(item.pControl, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
			InvalidateLayout();
			RefreshRuleLabels();
			SetDirty();
		}
	}

	void CharacterEditorRulesTab::RefreshRuleLabels()
	{
		for (size_t i = 0uz; i < _items.size(); ++i)
			_items[i].pControl->SetLabel(std::format("Rule #{}", i + 1uz));

		_pAddButton->SetEnabled(_items.size() < kMaxRuleCount);
	}

	void CharacterEditorRulesTab::OnCopyRule(size_t index)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			auto& item = *itFind;
			SDL_SetClipboardText(item.pControl->GetValue().data());
		}
	}

	void CharacterEditorRulesTab::OnPasteRule(size_t index)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			auto& item = *itFind;
			fig::string content = SDL_GetClipboardText();
			if (not content.empty())
			{
				item.pControl->SetValue(content);
				SetDirty();
			}
		}
	}

	EditorTabBase::SaveResult CharacterEditorRulesTab::OnSave() noexcept
	{
		auto rules = _items
			| std::views::transform([](auto&& i) { return fig::string { i.pControl->GetValue() }; })
			| std::views::filter([](auto&& s) { return not empty_or_whitespace(s); })
			| std::ranges::to<std::vector>();

		_pCharacter->ClearRules();
		_pCharacter->AppendRules(rules);
		return {};
	}

}