#pragma once

#include "Figment.h"
#include "ITabControl.h"

namespace fig::gui
{
	class TabControlGroup
	{
	public:
		TabControlGroup(ControlPtr pOwner);

		void AddControl(fig::observer_ptr<ITabControl> pControl);
		void RemoveControl(fig::observer_ptr<ITabControl> pControl);
		bool ProcessEvent(fig::event& event);
	
		bool FocusNext();
		bool FocusPrev();

	protected:
		bool Navigate(TabDirection direction, bool bLoopAround);

	private:
		std::vector<fig::observer_ptr<ITabControl>> _controls;
		fig::observer_ptr<TabControlGroup> _pParentGroup {};
	};
}