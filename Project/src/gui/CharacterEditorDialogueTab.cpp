#include <pch.h>
#include "gui/CharacterEditorDialogueTab.h"
#include "gui/CharacterDialogueWidget.h"
#include "gui/ButtonWithLabel.h"
#include "gui/Menu.h"
#include "gui/AppResources.h"
#include "data/Character.h"

using namespace fig::data;
using namespace fig::io;

namespace fig::gui
{
	size_t CharacterEditorDialogueTab::_nextEntryIndex {};
	static constexpr size_t kMaxEntryCount = 15uz;

	CharacterEditorDialogueTab::CharacterEditorDialogueTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool CharacterEditorDialogueTab::Initialize(CharacterEditorArgs args)
	{
		if (not (bool)args.pCharacter)
			return false;

		_pCharacter = args.pCharacter;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Example dialogue");

		CreateHint(this, pSizer, "Example dialogue contributes to the speech patterns of the character and may also influence the topic of conversation.");
		CreateHint(this, pSizer, "Examples should only include spoken dialogue, not actions or narration.");

		_pEntrySizer = new VerticalSizer();
		pSizer->Add(_pEntrySizer.get(), 0, SizerFlag::Expand | SizerFlag::Bottom, 8);

		CreateTabGroup();

		size_t count = 0uz;
		for (auto& dialogue : _pCharacter->GetExampleDialogue())
		{
			AddEntry(dialogue);
			++count;
		}
		for (; count < 1uz; ++count)
			AddEntry({});

		_pAddButton = CreateControl<ButtonWithLabel>("Add example");
		_pAddButton->SetHeight(35);
		_pAddButton->SetDelegate([this] { auto pEntry = AddEntry({}); pEntry->SetFocus(true); SetDirty(); });
		pSizer->Add(_pAddButton, 0);

		CreateHint(this, pSizer, std::format("You can add up to {} examples.", kMaxEntryCount));

		return true;
	}

	fig::observer_ptr<CharacterDialogueWidget> CharacterEditorDialogueTab::AppendControl(const fig::data::ExampleDialogue& example, size_t index)
	{
		auto pEntry = CreateControl<CharacterDialogueWidget>(example, _pCharacter.get());
		pEntry->SetButtonDelegate([this, index] { OnEntryMenu(index); });
		pEntry->SetValueChangedDelegate([this](auto&& _) { SetDirty(); });
		pEntry->SetMoveDelegate([this, index](auto&& dir) { OnMoveEntry(index, dir, IsShiftDown()); });
		_pEntrySizer->Add(pEntry, 0, SizerFlag::Expand | SizerFlag::Bottom, 12);

		_pTabGroup->AddControl(pEntry);
		return pEntry;
	}

	fig::observer_ptr<CharacterDialogueWidget> CharacterEditorDialogueTab::AddEntry(const fig::data::ExampleDialogue& exampleDialogue)
	{
		auto item = EntryItem {
			.index = _nextEntryIndex++,
		};

		item.pControl = AppendControl(exampleDialogue, item.index);
		_items.push_back(std::move(item));
		return item.pControl;
	}

	void CharacterEditorDialogueTab::OnEntryMenu(size_t entryIndex)
	{
		EntryItem* pItem = nullptr;
		size_t index {};
		if (auto itFind = std::ranges::find(_items, entryIndex, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
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
			.SetDelegate([this, entryIndex] { OnMoveEntry(entryIndex, -1, true); });
		menu.AddItem("Move up")
			.SetEnabled(index > 0uz)
			.SetDelegate([this, entryIndex] { OnMoveEntry(entryIndex, -1, false); });
		menu.AddItem("Move down")
			.SetEnabled(index + 1uz < _items.size())
			.SetDelegate([this, entryIndex] { OnMoveEntry(entryIndex, 1, false); });
		menu.AddItem("Move to bottom")
			.SetEnabled(index + 1uz < _items.size())
			.SetDelegate([this, entryIndex] { OnMoveEntry(entryIndex, 1, true); });
		menu.AddSeparator();
		menu.AddItem("Remove", Resource::ICON_DELETE)
			.SetDelegate([this, entryIndex] { RemoveEntry(entryIndex); });

		menu.Show();
	}

	void CharacterEditorDialogueTab::RemoveEntry(size_t index)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			auto& item = *itFind;
			_pEntrySizer->Remove(item.pControl);
			DestroyChild(item.pControl);
			_items.erase(itFind);
			SetDirty();
		}
	}

	void CharacterEditorDialogueTab::OnMoveEntry(size_t index, int32_t dir, bool bMaxDistance)
	{
		if (auto itFind = std::ranges::find(_items, index, [](auto&& a) { return a.index; }); itFind != std::ranges::cend(_items))
		{
			size_t index = std::distance(std::ranges::begin(_items), itFind);
			if (dir < 0 and bMaxDistance) // Move to top
			{
				if (index == 0uz)
					return;
				EntryItem item = *itFind;
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

				EntryItem item = *itFind;
				_items.erase(itFind);
				_items.insert(_items.cbegin() + _items.size(), item);
			}
			else if (dir > 0) // Move down
			{
				if (index + 1 >= _items.size())
					return;
				std::swap(_items[index], _items[index + 1]);
			}

			_pEntrySizer->RemoveAll();
			for (auto& item : _items)
			{
				_pEntrySizer->Add(item.pControl, 0, SizerFlag::Expand | SizerFlag::Bottom, 8);
				
				_pTabGroup->RemoveControl(item.pControl);
				_pTabGroup->AddControl(item.pControl);
			}
			InvalidateLayout();
			SetDirty();
		}
	}

	EditorTabBase::SaveResult CharacterEditorDialogueTab::OnSave() noexcept
	{
		auto examples = _items
			| std::views::transform([](auto&& i) -> ExampleDialogue { return i.pControl->GetValue(); })
			| std::views::filter([](auto&& ex) { return not (empty_or_whitespace(ex.input) or empty_or_whitespace(ex.response)); })
			| std::ranges::to<std::vector>();

		_pCharacter->SetExampleDialogue(examples);
		return {};
	}

}