#include <pch.h>
#include "gui/BaseButton.h"
#include "gui/GUIUtility.h"

using namespace fig::gui::util;

namespace fig::gui
{
	BaseButton::BaseButton(LayoutElement* pOwner) : _pOwner { pOwner }
	{
	}

	void BaseButton::SetDelegate(ButtonDelegate pDelegate) noexcept
	{
		_fn = pDelegate;
	}

	void BaseButton::SetEnabled(bool bEnabled) noexcept
	{
		if (bEnabled == (_state == ButtonState::Disabled))
		{
			SetButtonState(bEnabled ? ButtonState::Default : ButtonState::Disabled);
			_bMouseDown = false;
		}		
	}

	bool BaseButton::IsEnabled() const noexcept 
	{ 
		return _state != ButtonState::Disabled; 
	}

	void BaseButton::SetExpandedArea(Coord size) noexcept
	{
		_expand = std::max(size, 0);
	}

	bool BaseButton::HandleMouseEvents(const Event& event) noexcept
	{
		if (_state == ButtonState::Disabled)
			return false;

		auto& rect = _pOwner->GetRect();

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			auto motionEvent = event.motion;
			if (is_inside(rect, toI(motionEvent.x), toI(motionEvent.y), _expand))
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
			auto mouseEvent = event.button;

			if (not is_inside(rect, toI(mouseEvent.x), toI(mouseEvent.y), _expand))
				return false; // Ignore

			if (mouseEvent.down != _bMouseDown)
			{
				if (_bMouseDown and !mouseEvent.down and _fn)
					_fn(); // Click!

				SetButtonState(mouseEvent.down ? ButtonState::Pressed : ButtonState::Hover);
				_bMouseDown = mouseEvent.down;
				_bMouseInside = false;

				_bMouseDown ? OnButtonDown() : OnButtonUp();
			}
			return true;
		}

		return false;
	}

	void BaseButton::SetButtonState(ButtonState state)
	{
		_state = state;
		OnButtonState();
	}

	void BaseButton::DropState() noexcept
	{
		if (IsEnabled())
			SetButtonState(ButtonState::Default);
		_bMouseInside = false;
		_bMouseDown = false;
	}
}