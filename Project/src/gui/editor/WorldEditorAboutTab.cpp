#include <pch.h>
#include "gui/editor/WorldEditorAboutTab.h"
#include "gui/TextBox.h"
#include "gui/AppResources.h"
#include "data/World.h"

using namespace fig::data;

namespace fig::gui
{
	WorldEditorAboutTab::WorldEditorAboutTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool WorldEditorAboutTab::OnInit(WorldEditorArgs args)
	{
		if (not (bool)args.pWorld)
			return false;

		_pWorld = args.pWorld;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "About world");

		// About
		CreateLabel(this, pSizer, "Description");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pWorld->about), 4);
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(8);

		// Tags
		CreateLabel(this, pSizer, "Tags");
		_pTags = CreateTextBox(this, pSizer);
		_pTags->SetMaxWidth(620);
		_pTags->SetPlaceholder("Enter tags separated by commas");
		_pTags->SetText(encode_csv(_pWorld->GetTags()));

		// Author
		CreateLabel(this, pSizer, "Creator");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pWorld->creator))
			->SetMaxWidth(300);

		// Version
		CreateLabel(this, pSizer, "Version");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pWorld->version))
			->SetMaxWidth(120);

		return true;
	}

	EditorTabBase::SaveResult WorldEditorAboutTab::OnSave() noexcept
	{
		auto tags = decode_csv(_pTags->GetText())
			| std::ranges::to<std::unordered_set>()
			| std::ranges::to<std::vector>();
		_pWorld->SetTags(tags);
		return {};
	}
}