#include <pch.h>
#include "gui/ButtonBase.h"
#include "gui/GUIUtility.h"

using namespace fig::gui::util;

namespace fig::gui
{
	ButtonBase::ButtonBase(LayoutElement* pOwner) : _pOwner { pOwner }
	{
	}

	void ButtonBase::SetDelegate(ButtonDelegate pDelegate) noexcept
	{
		_fn = pDelegate;
	}

	void ButtonBase::SetEnabled(bool bEnabled) noexcept
	{
		if (bEnabled != (_state == State::Disabled))
		{
			_state = bEnabled ? State::Default : State::Disabled;
			_bMouseDown = false;
		}		
	}

	bool ButtonBase::IsEnabled() const noexcept 
	{ 
		return _state != State::Disabled; 
	}

	void ButtonBase::SetExpandedArea(float size) noexcept
	{
		_fExpand = std::max(size, 0.0f);
	}

	bool ButtonBase::HandleMouseEvents(const Event& event) noexcept
	{
		if (_state == State::Disabled)
			return false;

		auto& rect = _pOwner->GetRect();

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			auto motionEvent = event.motion;
			if (is_inside(rect, motionEvent.x, motionEvent.y, _fExpand))
			{
				if (not _bMouseInside)
				{
					_state = State::Hover;
					_bMouseInside = true;
					OnMouseEnter();
				}
			}
			else
			{
				if (_bMouseInside)
				{
					_state = State::Default;
					_bMouseInside = false;
					_bMouseDown = false;
					OnMouseExit();
				}
			}
			return false; // Don't consume event
		}

		if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN or event.type == SDL_EVENT_MOUSE_BUTTON_UP) and event.button.button == SDL_BUTTON_LEFT)
		{
			if (not _bMouseInside)
				return false; // Ignore

			auto mouseEvent = event.button;
			if (mouseEvent.down != _bMouseDown)
			{
				if (_bMouseDown and !mouseEvent.down and _fn)
					_fn(); // Click!

				_state = mouseEvent.down ? State::Pressed : State::Hover;
				_bMouseDown = mouseEvent.down;

				_bMouseDown ? OnButtonDown() : OnButtonUp();
			}
			return true;
		}

		return false;
	}

}