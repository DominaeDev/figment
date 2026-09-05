#include <pch.h>
#include "gui/SidePanelEditor.h"
#include "gui/AppResources.h"

namespace fig::gui
{
	SidePanelEditor::SidePanelEditor(ControlPtr pParent) : SidePanelContent(pParent)
	{
		auto pMenuButton = CreateControl<ButtonWithIcon>(Resource::ICON_ARROW_LEFT);
		pMenuButton->SetTheme(Theme::SidePanelButtonStyle);
		pMenuButton->SetX(3);
		pMenuButton->SetY((Constants::GUI::SidePanel::HeaderHeight - pMenuButton->GetHeight()) / 2);
		pMenuButton->SetDelegate([this]() { PushEvent(UserEvent::NavigateToHome); });
	}

	void SidePanelEditor::ShowExpanded()
	{
	}

	void SidePanelEditor::ShowCollapsed()
	{
	}

}