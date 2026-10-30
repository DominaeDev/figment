#pragma once

#include "gui/EditorTab.h"
#include "gui/StoryEditorArgs.h"

namespace fig::gui
{
	class StoryEditorAboutTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorAboutTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;

	protected:
		bool OnInit(StoryEditorArgs args) override;

	private:
		fig::observer_ptr<fig::data::Scenario> _pScenario {};
		fig::observer_ptr<TextBox> _pTags {};
	};
}