#include <pch.h>
#include "gui/Control.h"
#include "gui/TabControlGroup.h"

namespace fig::gui
{
	TabControlGroup::TabControlGroup(ControlPtr pOwner)
	{
		auto pParent = pOwner->GetParent();
		while (auto pParentControl = dynamic_cast<Control*>(pParent.get()))
		{
			if (_pParentGroup = pParentControl->GetTabGroup())
				break;
			pParent = pParent->GetParent();
		}

		for (auto& pChild : pOwner->GetChildren())
		{
			if (auto pTabControl = dynamic_cast<ITabControl*>(pChild))
				AddControl(pTabControl);
			if (auto pControl = dynamic_cast<Control*>(pChild))
			{
				if (auto pSubGroup = pControl->GetTabGroup())
					pSubGroup->_pParentGroup = this;
			}
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
		if (_pParentGroup)
			return _pParentGroup->ProcessEvent(event);

		if (IsUserEvent(event, UserEvent::TabNavigationForward))
		{
			if (Navigate(1, true))
				return true;
		}
		else if (IsUserEvent(event, UserEvent::TabNavigationBackward))
		{
			if (Navigate(-1, true))
				return true;
		}
		return false;
	}

	bool TabControlGroup::Navigate(TabDirection direction, bool bLoopAround)
	{
		if (auto index = find_index_if(_controls, [](auto&& pCtrl) { return pCtrl->IsFocused(); }); index != fig::npos)
		{
			auto prev_index = index;
			if (direction < 0)
			{
				if (_controls[index]->FocusPrev())
					return true;
				if (auto pControl = dynamic_cast<Control*>(_controls[index].get()))
				{
					if (auto pSubGroup = pControl->GetTabGroup(); pSubGroup and pSubGroup->FocusPrev())
						return true;
				}

				for (size_t tries = 0uz; tries < _controls.size(); ++tries)
				{
					if (index == 0uz)
					{
						if (bLoopAround)
							index = _controls.size() - 1uz;
						else
							return false;
					}
					else
						index--;

					if (_controls[index]->CanFocus())
						break;
				}
			}
			else if (direction > 0)
			{
				if (_controls[index]->FocusNext())
					return true;
				if (auto pControl = dynamic_cast<Control*>(_controls[index].get()))
				{
					if (auto pSubGroup = pControl->GetTabGroup(); pSubGroup and pSubGroup->FocusNext())
						return true;
				}

				for (size_t tries = 0uz; tries < _controls.size(); ++tries)
				{
					if (++index == _controls.size())
					{
						if (bLoopAround)
							index = 0uz;
						else 
							return false;
					}

					if (_controls[index]->CanFocus())
						break;
				}
			}

			_controls[prev_index]->SetFocus(false, direction);
			_controls[index]->SetFocus(true, direction);
			return true;
		}
		return false;
	}

	bool TabControlGroup::FocusNext()
	{
		if (Navigate(1, false))
			return true;
		return false;
	}

	bool TabControlGroup::FocusPrev()
	{
		if (Navigate(-1, false))
			return true;
		return false;
	}
}