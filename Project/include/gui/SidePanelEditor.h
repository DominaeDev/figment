#pragma once

#include "SidePanel.h"

namespace fig::gui
{
	class SidePanelEditor : public SidePanelContent
	{
	public:
		SidePanelEditor(ControlPtr pParent);

		void SetEditor(fig::observer_ptr<Editor> pEditor) noexcept;

		void ShowExpanded() override;
		void ShowCollapsed() override;

	private:
		fig::observer_ptr<Editor> _pEditor;
	};
}