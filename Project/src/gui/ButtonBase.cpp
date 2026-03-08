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
		if (bEnabled != (_state == ButtonState::Disabled))
		{
			SetButtonState(bEnabled ? ButtonState::Default : ButtonState::Disabled);
			_bMouseDown = false;
		}		
	}

	bool ButtonBase::IsEnabled() const noexcept 
	{ 
		return _state != ButtonState::Disabled; 
	}

	void ButtonBase::SetExpandedArea(float size) noexcept
	{
		_fExpand = std::max(size, 0.0f);
	}

	bool ButtonBase::HandleMouseEvents(const Event& event) noexcept
	{
		if (_state == ButtonState::Disabled)
			return false;

		auto& rect = _pOwner->GetRect();

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			auto motionEvent = event.motion;
			if (is_inside(rect, motionEvent.x, motionEvent.y, _fExpand))
			{
				if (not _bMouseInside)
				{
					SetButtonState(ButtonState::Hover);
					_bMouseInside = true;
					OnMouseEnter();
				}
			}
			else
			{
				if (_bMouseInside)
				{
					SetButtonState(ButtonState::Default);
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

				SetButtonState(mouseEvent.down ? ButtonState::Pressed : ButtonState::Hover);
				_bMouseDown = mouseEvent.down;

				_bMouseDown ? OnButtonDown() : OnButtonUp();
			}
			return true;
		}

		return false;
	}

	void ButtonBase::SetButtonState(ButtonState state)
	{
		_state = state;
		OnButtonState();
	}

}