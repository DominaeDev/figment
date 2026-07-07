#include <pch.h>
#include "gui/VerticalScrollBar.h"
#include "gui/TexturedBorder.h"
#include "gui/AppResources.h"
#include "gui/ScrollPanel.h"

namespace fig::gui
{
	constexpr Coord Margin = 8;
	constexpr std::array<float, 2> HandleAlpha { 0.25f, 0.5f };

	VerticalScrollBar::VerticalScrollBar(LayoutElement* pParent) : Control(pParent)
	{
		SetBackgroundColor(Colors::Transparent);

		auto pHandle = CreateControl<TexturedBorder>(AppResources::GetTexture(TextureType::ROUNDED_BACKGROUND_6PX), 8);
		pHandle->SetWidth(8);
		pHandle->SetCornerScale(0.5f);
		pHandle->SetForegroundColor(Colors::Black.WithAlpha(HandleAlpha[0]));
		_pHandle = pHandle;
	}

	void VerticalScrollBar::SetScroll(ScrollPanel& scrollPanel, float fPosition, Coord maxExtent)
	{
		if (_bScrolling)
			return; // Ignore

		_fPosition = fPosition;
		_fExtent = toF(maxExtent);
		_bDirty = true;
		_pScrollPanel = &scrollPanel;
	}

	void VerticalScrollBar::OnUpdate(float fElapsed)
	{
		if (_bDirty)
		{
			RefreshHandle();
			_bDirty = false;
		}
	}

	void VerticalScrollBar::RefreshHandle()
	{
		int32_t pageHeight = GetHeight();
		float fScrollRange = _fExtent;
		if (fScrollRange <= 0.0f)
		{
			SetVisible(false);
			return;
		}
		
		SetVisible(true);

		int32_t handleSize = std::clamp(toI(std::min(toF(pageHeight) / fScrollRange, 1.0f) * pageHeight), 60, 300);
		_pHandle->SetY(Margin + toI((_fPosition / _fExtent) * (pageHeight - handleSize - Margin * 2)));
		_pHandle->SetHeight(handleSize);
		_pHandle->CenterHorizontally();
	}

	EventResult VerticalScrollBar::OnEvent(Event& event)
	{
		switch (event.type)
		{
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event.button.button == SDL_BUTTON_LEFT)
				return HandleMouseDown(toI(event.button.x), toI(event.button.y)) ? EventResult::Handled : EventResult::Pass;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event.button.button == SDL_BUTTON_LEFT)
				HandleMouseUp(toI(event.button.x), toI(event.button.y)) ? EventResult::Handled : EventResult::Pass;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			return HandleMouseMotion(toI(event.motion.x), toI(event.motion.y)) ? EventResult::Handled : EventResult::Pass;
		}

		return EventResult::Pass;
	}

	bool VerticalScrollBar::HandleMouseDown(int32_t x, int32_t y)
	{
		auto& rect = GetRect();

		if (is_inside(rect, x, y))
		{
			_pHandle->SetForegroundColor(_pHandle->GetForegroundColor().WithAlpha(HandleAlpha[1]));
			_bScrolling = true;
			return true;
		}
		return false;
	}

	bool VerticalScrollBar::HandleMouseUp(int32_t x, int32_t y)
	{
		if (_bScrolling)
		{
			_pHandle->SetForegroundColor(_pHandle->GetForegroundColor().WithAlpha(HandleAlpha[0]));
			_bScrolling = false;
			_bDirty = true;
		}
		return false;
	}

	bool VerticalScrollBar::HandleMouseMotion(int32_t x, int32_t y)
	{
		if (!_bScrolling or !_pScrollPanel)
			return false;

		y -= GetAbsoluteY() + Margin + _pHandle->GetHeight() / 2;
		_fPosition = _fExtent * std::clamp(toF(y) / (GetHeight() - Margin * 2 - _pHandle->GetHeight()), 0.0f, 1.0f);
		_pScrollPanel->ScrollTo(_fPosition);

		RefreshHandle();
		return true;
	}

}