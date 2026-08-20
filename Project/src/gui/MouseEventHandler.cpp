#include <pch.h>
#include "gui/MouseEventHandler.h"
#include "gui/GUIUtility.h"

namespace fig::gui
{
	MouseEventHandler::MouseEventHandler(ControlPtr pParent) : 
		_pOwner(pParent)
	{
	}

	void MouseEventHandler::SetDelegate(MouseClickedDelegate pDelegate) noexcept
	{
		_fnClicked = pDelegate;
	}

	void MouseEventHandler::SetMouseEnterDelegate(MouseEnterDelegate pDelegate) noexcept
	{
		_fnEnter = pDelegate;
	}

	void MouseEventHandler::SetMouseExitDelegate(MouseExitDelegate pDelegate) noexcept
	{
		_fnExit = pDelegate;
	}

	void MouseEventHandler::SetMouseDownDelegate(MouseDownDelegate pDelegate) noexcept
	{
		_fnDown = pDelegate;
	}

	void MouseEventHandler::SetMouseUpDelegate(MouseUpDelegate pDelegate) noexcept
	{
		_fnUp= pDelegate;
	}

	void MouseEventHandler::SetClickableRegion(const fig::rect& rect, fig::coord expand) noexcept
	{
		_region = rect;
		_expand = expand;
	}

	void MouseEventHandler::Enable(bool bEnable) noexcept
	{
		if (bEnable and _state == ButtonState::Disabled)
		{
			_state = ButtonState::Default;
			OnButtonState();
		}
		else if (not bEnable and _state != ButtonState::Disabled)
		{
			_state = ButtonState::Disabled;
			OnButtonState();
		}

		_bEnabled = bEnable;
		_bMouseDown = false;
		_bMouseInside = false;
	}

	void MouseEventHandler::SetExpandSize(fig::coord size) noexcept
	{
		_expand = std::max(size, 0);
	}

	EventResult MouseEventHandler::HandleMouseEvents(const fig::event& event) noexcept
	{
		if (not (_bEnabled and (bool)_pOwner))
			return EventResult::Pass;

		fig::rect rect = _pOwner->GetRect();
		if (_region.w != 0 or _region.h != 0)
		{
			rect.x += _region.x;
			rect.y += _region.y;
			rect.w = _region.w;
			rect.h = _region.h;
		}

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			auto motionEvent = event.motion;
			if (is_inside(rect, toI(motionEvent.x), toI(motionEvent.y), _expand))
			{
				if (not _bMouseInside and _state != ButtonState::Pressed)
				{
					SetButtonState(ButtonState::Hover);
					_bMouseInside = true;
					if (_fnEnter)
						_fnEnter();
					OnMouseEnter();
				}
			}
			else
			{
				if (_bMouseInside or _state == ButtonState::Pressed)
				{
					SetButtonState(ButtonState::Default);
					_bMouseInside = false;
					_bMouseDown = false;
					if (_fnExit)
						_fnExit();
					OnMouseExit();
				}
			}
			return EventResult::Continue;
		}

		if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN or event.type == SDL_EVENT_MOUSE_BUTTON_UP) and event.button.button == SDL_BUTTON_LEFT)
		{
			auto mouseEvent = event.button;

			if (not is_inside(rect, toI(mouseEvent.x), toI(mouseEvent.y), _expand))
				return EventResult::Pass;

			if (mouseEvent.down != _bMouseDown)
			{
				if (_bMouseDown and !mouseEvent.down) // Click!
				{
					if (_fnClicked)
						_fnClicked();
					OnClicked();
				}

				SetButtonState(mouseEvent.down ? ButtonState::Pressed : ButtonState::Default);
				_bMouseDown = mouseEvent.down;
				_bMouseInside = false;

				if (_bMouseDown)
				{
					if (_fnDown)
						_fnDown(fig::point { toI(mouseEvent.x), toI(mouseEvent.y) });
					OnButtonDown();
				}
				else
				{
					if (_fnUp)
						_fnUp(fig::point { toI(mouseEvent.x), toI(mouseEvent.y) });
					OnButtonUp();
				}
				return EventResult::Handled;
			}
		}

		return EventResult::Pass;
	}

	void MouseEventHandler::SetButtonState(ButtonState state)
	{
		if (not _bEnabled)
			return;

		if (_state != state)
		{
			_state = state;
			OnButtonState();
		}
	}

	void MouseEventHandler::DropState() noexcept
	{
		_bMouseInside = false;
		_bMouseDown = false;
		_state = _bEnabled ? ButtonState::Default : ButtonState::Disabled;
		OnButtonState();
	}
}