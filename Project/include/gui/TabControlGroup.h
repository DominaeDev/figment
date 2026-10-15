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
	
	protected:
		bool Navigate(int32_t direction);
	private:
		std::vector<fig::observer_ptr<ITabControl>> _controls;
	};
}