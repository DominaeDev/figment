#include <pch.h>
#include "gui/StoryEditorInfoTab.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ButtonWithLabel.h"
#include "gui/ButtonWithLabelAndIcon.h"
#include "gui/AppResources.h"
#include "gui/HorizontalLine.h"
#include "data/Scenario.h"
#include "data/Character.h"
#include "util/StringUtils.h"

using namespace fig::data;

namespace fig::gui
{
	StoryEditorInfoTab::StoryEditorInfoTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool StoryEditorInfoTab::OnInit(StoryEditorArgs args)
	{
		if (not (bool)args.pScenario)
			return false;

		_pScenario = args.pScenario;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "Story parameters");

		// Name
		CreateLabel(this, pSizer, "Story name");
		_pName = CreateTextBox(this, pSizer, [this](auto&& value) {
			_pScenario->name = value;
			_pTitle->SetPlaceholder(_pName->GetText());
		});
		_pName->SetMaxWidth(300);

		// Title
		CreateLabel(this, pSizer, "Chat title");
		_pTitle = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->title));
		_pTitle->SetMaxWidth(300);
		_pTitle->SetPlaceholder(_pName->GetText());

		// About
		CreateLabel(this, pSizer, "Description");
		auto pDescription = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pScenario->about), 4);
		pDescription->SetMaxWidth(620);
		pDescription->EnableAutoSize(true);
		pDescription->SetMinRows(2);
		pDescription->SetMaxRows(8);
		CreateHint(this, pSizer, "The story description is displayed alongside the chat to inform users what this story is about. Story content should go into scenes.");

		// Genres
		CreateLabel(this, pSizer, "Genres");
		_pGenres = CreateTextBox(this, pSizer);
		_pGenres->SetMaxWidth(620);
		_pGenres->SetPlaceholder("Enter one or more genre, separated by commas");
		_pGenres->SetText(encode_csv(_pScenario->_genres));

		return true;
	}

	void StoryEditorInfoTab::OnTabSelected()
	{
		if (_arguments.pCharacter)
		{
			_pName->SetText(std::format("{}'s story", _arguments.pCharacter->name.GetSpokenName()));
			_pName->SetEnabled(false);

			_pTitle->SetPlaceholder(_pName->GetText());
		}
	}

	EditorTabBase::SaveResult StoryEditorInfoTab::OnSave() noexcept
	{
		auto genres = decode_csv(_pGenres->GetText())
			| std::ranges::to<std::unordered_set>()
			| std::ranges::to<std::vector>();
		_pScenario->_genres = genres;

		auto tags = decode_csv(_pTags->GetText())
			| std::ranges::to<std::unordered_set>()
			| std::ranges::to<std::vector>();
		_pScenario->SetTags(tags);
		return {};
	}
}