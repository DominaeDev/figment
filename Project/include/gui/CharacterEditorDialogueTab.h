#pragma once

#include "gui/EditorTab.h"
#include "gui/CharacterEditorArgs.h"
#include "io/XmlData.h"

namespace fig::data
{
	class Character;
	struct ExampleDialogue;
}

namespace fig::gui
{
	class CharacterDialogueWidget;

	class CharacterEditorDialogueTab : public EditorTab<CharacterEditorArgs>
	{
	public:
		CharacterEditorDialogueTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;
	
	protected:
		bool OnInit(CharacterEditorArgs args) override;

	private:
		fig::observer_ptr<CharacterDialogueWidget> AddEntry(const fig::data::ExampleDialogue& example);
		fig::observer_ptr<CharacterDialogueWidget> AppendControl(const fig::data::ExampleDialogue& example, size_t index);
		void RemoveEntry(size_t index);
		void OnMoveEntry(size_t index, int32_t dir, bool bMaxDistance);

		void OnEntryMenu(size_t index);
		void OnCopyEntry(size_t index);
		void OnPasteEntry(size_t index);

		fig::observer_ptr<fig::data::Character> _pCharacter {};

		struct EntryItem
		{
			size_t index;
			fig::observer_ptr<CharacterDialogueWidget> pControl;
		};

		std::vector<EntryItem> _items;
		static size_t _nextEntryIndex;
		SizerPtr _pEntrySizer {};
		fig::observer_ptr<ButtonWithLabel> _pAddButton;
	};
}