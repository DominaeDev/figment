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

	bool StoryEditorAboutTab::Initialize(StoryEditorArgs args)
	{
		if (not (bool)args.pScenario)
			return false;

		_pScenario = args.pScenario;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "About story");

		if (not args.bCharacterStory)
		{
			// Name
			CreateLabel(this, pSizer, "Story name");
			auto pName = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->name));
			pName->SetMaxWidth(300);
		}

		// Title
		CreateLabel(this, pSizer, "Chat title");
		auto pTitle = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->title));
		pTitle->SetMaxWidth(300);

		// About
		CreateLabel(this, pSizer, "Description");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->about), 4);
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(8);
		pDescription->SetPlaceholder("Please provide a brief description about this story");

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
		return {};
	}
}