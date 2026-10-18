#include <pch.h>
#include "gui/Control.h"
#include "gui/TabControlGroup.h"

namespace fig::gui
{
	TabControlGroup::TabControlGroup(ControlPtr pOwner)
	{
		for (auto& pChild : pOwner->GetChildren())
		{
			if (auto pTabControl = dynamic_cast<ITabControl*>(pChild))
				AddControl(pTabControl);
		}
	}

	void TabControlGroup::AddControl(fig::observer_ptr<ITabControl> pControl)
	{
		_controls.push_back(pControl);
	}

	void TabControlGroup::RemoveControl(fig::observer_ptr<ITabControl> pControl)
	{
		if (auto e = std::ranges::remove(_controls, pControl); e.begin() != e.end())
			_controls.erase(e.begin(), e.end());
	}

	bool TabControlGroup::ProcessEvent(fig::event& event)
	{
		if (IsUserEvent(event, UserEvent::TabNavigationForward))
		{
			if (Navigate(1))
				return true;
		}
		else if (IsUserEvent(event, UserEvent::TabNavigationBackward))
		{
			if (Navigate(-1))
				return true;
		}
		return false;
	}

	bool TabControlGroup::Navigate(int32_t direction)
	{
		if (auto index = find_index_if(_controls, [](auto&& pCtrl) { return pCtrl->IsFocused(); }); index != fig::npos)
		{
			auto prev_index = index;
			if (direction < 0)
			{
				for (size_t tries = 0uz; tries < _controls.size(); ++tries)
				{
					if (index == 0uz)
						index = _controls.size() - 1uz;
					else
						index--;

					if (_controls[index]->CanFocus())
						break;
				}
			}
			else if (direction > 0)
			{
				for (size_t tries = 0uz; tries < _controls.size(); ++tries)
				{
					if (++index == _controls.size())
						index = 0uz;

					if (_controls[index]->CanFocus())
						break;
				}
			}

			_controls[prev_index]->SetFocus(false);
			_controls[index]->SetFocus(true);
			return true;
		}
		return false;
	}
}