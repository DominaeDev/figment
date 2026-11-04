#include <pch.h>
#include "gui/editor/WorldEditorInfoTab.h"
#include "gui/TextBox.h"
#include "gui/AppResources.h"
#include "data/World.h"
#include "data/Character.h"

using namespace fig::data;

namespace fig::gui
{
	WorldEditorInfoTab::WorldEditorInfoTab(ControlPtr pParent) : EditorTab(pParent)
	{
		SetMaxWidth(1280);
	}

	bool WorldEditorInfoTab::OnInit(WorldEditorArgs args)
	{
		if (not (bool)args.pWorld)
			return false;

		_pWorld = args.pWorld;

		auto pSizer = SetSizer<VerticalSizer>();

		CreateHeader(this, pSizer, "World parameters");

		// Name
		CreateLabel(this, pSizer, "World name");
		_pName = CreateTextBox(this, pSizer, ValueBinding<fig::string>(&_pWorld->name));
		_pName->SetText(_pWorld->name);
		_pName->SetMaxWidth(300);

		return true;
	}
}