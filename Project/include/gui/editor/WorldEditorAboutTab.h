#pragma once

#include "gui/editor/EditorTab.h"
#include "gui/editor/WorldEditorArgs.h"

namespace fig::gui
{
	class WorldEditorAboutTab : public EditorTab<WorldEditorArgs>
	{
	public:
		WorldEditorAboutTab(ControlPtr pParent);

		SaveResult OnSave() noexcept override;

	protected:
		bool OnInit(WorldEditorArgs args) override;

	private:
		fig::observer_ptr<fig::data::World> _pWorld{};
		fig::observer_ptr<TextBox> _pTags {};
	};
}