#pragma once

#include "SidePanel.h"

namespace fig::gui
{
	class SidePanelEditor : public SidePanelContent
	{
	public:
		SidePanelEditor(ControlPtr pParent);

		void ShowExpanded() override;
		void ShowCollapsed() override;
	};
}