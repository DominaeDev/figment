#pragma once

#include "gui/editor/EditorTab.h"
#include "gui/editor/WorldEditorArgs.h"

namespace fig::gui
{
	class WorldEditorInfoTab : public EditorTab<WorldEditorArgs>
	{
	public:
		WorldEditorInfoTab(ControlPtr pParent);

	protected:
		bool OnInit(WorldEditorArgs args) override;

	private:
		fig::observer_ptr<fig::data::World> _pWorld {};
		fig::observer_ptr<TextBox> _pName {};
	};
}