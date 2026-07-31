#include <pch.h>
#include "gui/Slider.h"
#include "gui/TexturedBorder.h"
#include "gui/AppResources.h"
#include "gui/FillParentSizer.h"

namespace fig::gui
{
	constexpr fig::coord Margin = 4;

	Slider::Slider(ControlPtr pParent) : Area(pParent)
	{
		SetSize(200, 9);

		_pBar = CreateControl<TexturedBorder>(Resource::SLIDER_BAR_BG, 4);
		_pBar->SetForegroundColor(0xf4f2ec80_rgba);
		_pFill = _pBar->CreateControl<TexturedBorder>(Resource::SLIDER_BAR_BG, 4);
		_pFill->SetForegroundColor(0x57caff_rgb);

		auto pBarBorder = _pBar->CreateControl<TexturedBorder>(Resource::SLIDER_BAR_BORDER, 4);
		pBarBorder->SetForegroundColor(Color::LineColor);
		_pBar->SetSizer<FillParentSizer>()->Add(pBarBorder);

		_pThumb = CreateControl<Image>(Resource::SLIDER_THUMB_BG, Color::AppBackground);
		_pThumb->CenterVertically();
		auto pThumbBorder = _pThumb->CreateControl<Image>(Resource::SLIDER_THUMB_BORDER, Color::LineColor);
		pThumbBorder->FillParent();
		_thumbHalfSize = _pThumb->GetTextureSize().x / 2;

		SetValue(0.5f);
	}

	Slider::Slider(ControlPtr pParent, float fMin, float fMax) : Slider(pParent)
	{
		_fMin = fMin;
		_fMax = fMax;
	}

	void Slider::SetValue(float value)
	{
		_value = value;
		_bInvalidBar = true;
	}

	void Slider::OnUpdate(float fElapsed)
	{
		if (_bInvalidBar)
		{
			RefreshBar();
			_bInvalidBar = false;
		}
	}

	EventResult Slider::OnEvent(fig::event& event)
	{
		auto fnDrag = [&](fig::coord mx) {
			auto& rect = GetRect();
			fig::coord x = mx - (rect.x + Margin) - 1;
			fig::coord width = rect.w - Margin * 2;

			float value = width > 0 ? std::clamp(x / toF(width), 0.0f, 1.0f) : 1.0f;

			if (not flt_eq(value, _value))
			{
				SetValue(value);
				if (_fnDelegate)
					_fnDelegate(_fMin + _value * (_fMax - _fMin));
			}
		};

		if (event.type == SDL_EVENT_MOUSE_MOTION)
		{
			auto motionEvent = event.motion;
			if (_bDragging) // Dragging
			{
				fnDrag(toI(motionEvent.x));
				return EventResult::Continue;
			}
			else // not dragging
			{
				auto rect = GetRect();
				if (is_inside(rect, toI(motionEvent.x), toI(motionEvent.y), 6))
				{
					if (not (_bHovering))
					{
						_bHovering = true;
						return EventResult::Continue;
					}
				}
				else if (_bHovering)
				{
					_bHovering = false;
					return EventResult::Continue;
				}
			}
		}
		else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
		{
			if (_bHovering)
			{
				_bDragging = true;
				_bHovering = false;
				fnDrag(toI(event.button.x));
				return EventResult::Handled;
			}
		}
		else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT)
		{
			_bHovering = false;
			_bDragging = false;
			return EventResult::Continue;
		}

		return EventResult::Pass;
	}


	void Slider::OnSize()
	{
		_bInvalidBar = true;
	}

	void Slider::RefreshBar()
	{
		auto& rect = GetRect();
		_pBar->SetRect(rect);

		auto fillRect = rect;
		fillRect.w = Margin * 2 + static_cast<fig::coord>(toF(fillRect.w - Margin * 2) * std::clamp(_value, 0.0f, 1.0f));
		_pFill->SetRect(fillRect);

		_pThumb->SetX(fillRect.w - _thumbHalfSize - Margin);
	}

	fig::rect Slider::GetThumbRect() const noexcept
	{
		auto rect = GetRect();

		fig::coord x = rect.x + Margin * 2 + static_cast<fig::coord>(toF(rect.w - Margin * 2) * std::clamp(_value, 0.0f, 1.0f)) - _thumbHalfSize;
		fig::coord y = rect.y + rect.h / 2 - _thumbHalfSize;
		return fig::rect { x, y, _thumbHalfSize * 2, _thumbHalfSize * 2 };
	}

}