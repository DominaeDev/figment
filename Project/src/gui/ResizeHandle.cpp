#include <pch.h>
#include "gui/ResizeHandle.h"
#include "gui/GUIUtility.h"
#include "gui/LineBorderRenderer.h"

namespace fig::gui
{
	ResizeHandle::ResizeHandle(ControlPtr pParent, Direction direction) : Control(pParent),
		_direction { direction }
	{
		FillParent();

		SetBorderRenderer<LineBorderRenderer>(Color::LineColor, direction);
	}

	void ResizeHandle::OnUpdate(float fElapsed)
	{
		if (not _bResizing)
		{
			constexpr float kFadeSpeed = 10.0f;
			float fTargetAlpha = _bHovering ? 1.0f : 0.0f;

			if (!flt_eq(_fAlpha, fTargetAlpha))
			{
				_fAlpha = std::clamp(_fAlpha + (fTargetAlpha - _fAlpha) * kFadeSpeed * fElapsed, 0.0f, 1.0f);
				if (std::abs(fTargetAlpha - _fAlpha) < 0.05f)
					_fAlpha = fTargetAlpha;
			}
		}
	}

	void ResizeHandle::Render(fig::renderer_ptr pRenderer)
	{
		if (_bHovering or _bResizing)
		{
			auto lineColor = _bResizing ? 0x0060C0_rgb : Color::LineColor.WithAlpha(_fAlpha);
			auto drawRect = GetHandleRect();
			SDL_SetRenderDrawBlendMode(pRenderer, SDL_BLENDMODE_BLEND);
			SDL_SetRenderDrawColor(pRenderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);
			SDL_RenderFillRect(pRenderer, &drawRect);
		}
		else
		{
			DrawBorder(pRenderer);
		}
	}

	fig::rectf ResizeHandle::GetHandleRect() const noexcept
	{
		float left = toF(GetAbsoluteX());
		float right = toF(GetAbsoluteX() + GetWidth());
		float top = toF(GetAbsoluteY());
		float bottom = toF(GetAbsoluteY() + GetHeight());
		switch (_direction)
		{
			case Direction::East:
				return rectf { right - 2, top, 3, bottom - top };
			case Direction::West:
				return rectf { left - 1, top, 3, bottom - top };
			default:
				return {}; //! @todo
		}
	}

	EventResult ResizeHandle::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			auto motionEvent = event.motion;
			if (not _bResizing)
			{
				auto handleRect = GetHandleRect();
				if (is_inside(handleRect, motionEvent.x, motionEvent.y, 6.0f))
				{
					if (not (_bHovering))
					{
						_bHovering = true;
						PushEvent(UserEvent::PushCursor, Cursor::ResizeHorizontal);
						return EventResult::Continue;
					}
				}
				else if (_bHovering)
				{
					_bHovering = false;
					PushEvent(UserEvent::PopCursor, Cursor::ResizeHorizontal);
					return EventResult::Continue;
				}
			}
			else // Resizing
			{
				auto mx = fig::coord(motionEvent.x);
				auto my = fig::coord(motionEvent.y);
				
				fig::coord size;
				switch (_direction)
				{
				case Direction::East:
					size = mx - _prevRect.x;
					break;
				case Direction::West:
					size = (_prevRect.x + _prevRect.w) - mx;
					break;
				case Direction::North:
					size = my - _prevRect.y;
					break;
				case Direction::South:
					size = (_prevRect.y + _prevRect.h) - my;
					break;
				default:
					return EventResult::Pass;
				}

				if (_currSize != size)
				{
					_currSize = size;
					if (_fnOnResize)
						_fnOnResize(_currSize);
				}
			}
		}
		else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
		{
			if (event.button.clicks == 2)
			{
				auto handleRect = GetHandleRect();
				if (is_inside(handleRect, event.button.x, event.button.y, 6.0f))
				{
					_bHovering = false;
					_bResizing = false;
					PushEvent(UserEvent::PopCursor, Cursor::ResizeHorizontal);
					if (_fnOnClick)
						_fnOnClick();
					return EventResult::Handled;
				}
			}

			if (_bHovering)
			{
				auto parentRect = GetParent()->GetRect();
				switch (_direction)
				{
				case Direction::East:
				case Direction::West:
					_prevSize = parentRect.w;
					break;
				case Direction::North:
				case Direction::South:
					_prevSize = parentRect.h;
					break;
				}

				_bResizing = true;
				_bHovering = false;
				_currSize = _prevSize;
				_prevRect = GetParent()->GetRect();
				return EventResult::Handled;
			}
		}
		else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
		{
			if (_bResizing)
			{
				_bResizing = false;
				PushEvent(UserEvent::PopCursor, Cursor::ResizeHorizontal);
			}
			else if (_bHovering)
			{
				_bHovering = false;
				PushEvent(UserEvent::PopCursor, Cursor::ResizeHorizontal);
			}
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}
}