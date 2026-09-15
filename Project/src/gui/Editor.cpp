#include <pch.h>
#include "gui/Editor.h"

namespace fig::gui
{
	Editor::Editor(ControlPtr pParent) : Control(pParent)
	{
		_pTabSizer = SetSizer<VerticalSizer>();
	}

	void Editor::SelectTab(size_t index)
	{
		for (size_t i = 0uz; i < _tabs.size(); ++i)
			EnableTab(_tabs[i], i == index);
		InvalidateLayout();
		LayoutNow();
	}

	void Editor::EnableTab(EditorTabBase* pTab, bool bEnabled)
	{
		if (pTab)
		{
			pTab->EnableLayout(bEnabled);
			pTab->SetVisible(bEnabled);
			pTab->SetEnabled(bEnabled);
		}
	}



}