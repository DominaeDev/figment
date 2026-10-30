#include <pch.h>
#include "gui/StoryEditorAboutTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "data/Scenario.h"
#include "util/StringUtils.h"

using namespace fig::data;

namespace fig::gui
{
	StoryEditorAboutTab::StoryEditorAboutTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool StoryEditorAboutTab::OnInit(StoryEditorArgs args)
	{
		if (not (bool)args.pScenario)
			return false;

		_pScenario = args.pScenario;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "About story");

		// About
//		CreateLabel(this, pSizer, "Description");
//		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->about), 4);
//		pDescription->SetMaxWidth(620);
//		pDescription->EnableAutoSize(true);
//		pDescription->SetMinRows(2);
//		pDescription->SetMaxRows(8);
//		pDescription->SetPlaceholder("Please provide a brief description about this story");

		// Tags
		CreateLabel(this, pSizer, "Tags");
		_pTags = CreateTextBox(this, pSizer);
		_pTags->SetMaxWidth(620);
		_pTags->SetPlaceholder("Enter tags separated by commas");
		_pTags->SetText(encode_csv(_pScenario->GetTags()));

		// Author
		CreateLabel(this, pSizer, "Creator");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->creator))
			->SetMaxWidth(300);

		// Version
		CreateLabel(this, pSizer, "Version");
		CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->version))
			->SetMaxWidth(120);

		return true;
	}

	EditorTabBase::SaveResult StoryEditorAboutTab::OnSave() noexcept
	{
		auto tags = decode_csv(_pTags->GetText())
			| std::ranges::to<std::unordered_set>()
			| std::ranges::to<std::vector>();
		_pScenario->SetTags(tags);
		return {};
	}
}