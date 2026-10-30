#pragma once

#include "gui/EditorTab.h"
#include "gui/StoryEditorArgs.h"

namespace fig::gui
{
	class StoryEditorInfoTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorInfoTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;
	
	protected:
		bool OnInit(StoryEditorArgs args) override;
		void OnTabSelected() override;

	private:
		fig::observer_ptr<fig::data::Scenario> _pScenario {};
		fig::observer_ptr<TextBox> _pName {};
		fig::observer_ptr<TextBox> _pTitle {};
		fig::observer_ptr<TextBox> _pGenres {};
		fig::observer_ptr<TextBox> _pTags {};
	};
}