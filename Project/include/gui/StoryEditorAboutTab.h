#pragma once

#include "gui/EditorTab.h"
#include "gui/StoryEditorArgs.h"

namespace fig::gui
{
	class StoryEditorAboutTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorAboutTab(ControlPtr pParent);

		bool Initialize(StoryEditorArgs args) override;
		SaveResult OnSave() noexcept override;

	private:
		fig::observer_ptr<fig::data::Scenario> _pScenario {};
		fig::observer_ptr<TextBox> _pTags {};
	};
}