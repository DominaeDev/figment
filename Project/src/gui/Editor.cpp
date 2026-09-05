#include <pch.h>
#include "gui/Editor.h"

namespace fig::gui
{
	Editor::Editor(ControlPtr pParent) : Control(pParent)
	{
		_pPageSizer = SetSizer<VerticalSizer>();
	}

	void Editor::SelectPage(size_t index)
	{
		for (size_t i = 0uz; i < _pages.size(); ++i)
			EnablePage(_pages[i], i == index);
	}

	void Editor::EnablePage(EditorPage* pPage, bool bEnabled)
	{
		if (pPage)
		{
			pPage->EnableLayout(bEnabled);
			pPage->SetVisible(bEnabled);
			pPage->SetEnabled(bEnabled);
		}
	}

}