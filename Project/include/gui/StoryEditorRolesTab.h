#pragma once

#include "gui/EditorTab.h"
#include "gui/StoryEditorArgs.h"

namespace fig::gui
{
	class StoryEditorRolesTab : public EditorTab<StoryEditorArgs>
	{
	public:
		StoryEditorRolesTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;

	protected:
		bool OnInit(StoryEditorArgs args) override;
		void CreateRole(SizerPtr pSizer, fig::chat::Role role);

	private:
		fig::observer_ptr<fig::data::Scenario> _pScenario {};
	};
}