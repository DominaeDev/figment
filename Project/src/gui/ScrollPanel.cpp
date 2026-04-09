#include <pch.h>
#include "gui/ScrollPanel.h"
#include "gui/GUIUtility.h"
#include "gui/VerticalScrollBar.h"

namespace fig::gui
{
	ScrollPanel::ScrollPanel(LayoutElement* pParent, bool bScrollBar) : Control(pParent)
	{
		EnableCulling(true);
		EnableClipping(true);

		if (bScrollBar)
		{
			_pScrollBar = new VerticalScrollBar(this);
			_pScrollBar->SetWidth(16);
			_pScrollBar->SetHeight(GetHeight());
			_pScrollBar->SetX(GetWidth() - _pScrollBar->GetWidth());
			RemoveChild(_pScrollBar);
		}
	}

	ScrollPanel::~ScrollPanel()
	{
		if (_pScrollBar)
			delete _pScrollBar;
	}

	void ScrollPanel::Render(Renderer* pRenderer)
	{
		Control::Render(pRenderer);

		if (_pScrollBar and GetVisible() and not IsCulled())
			_pScrollBar->Render(pRenderer);
	}

	bool ScrollPanel::OnEvent(Event& event)
	{
		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			return HandleMouseWheel(event.wheel);
		}

		if (_pScrollBar)
			return _pScrollBar->OnEvent(event);

		return false;
	}

	void ScrollPanel::OnUpdate(float fElapsed)
	{
		// Smooth scrolling
		if (_fScrollY != _fTargetScrollY)
		{
			_fScrollY += (_fTargetScrollY - _fScrollY) * Constants::GUI::MouseScrollSmoothing * fElapsed;

			if (std::abs(_fTargetScrollY - _fScrollY) < 0.5f)
				_fScrollY = _fTargetScrollY;
			else
				InvalidateLayout();
		}

		if (_pScrollBar)
			return _pScrollBar->OnUpdate(fElapsed);
	}

	bool ScrollPanel::HandleMouseWheel(SDL_MouseWheelEvent event)
	{
		Pointf pt = { event.mouse_x, event.mouse_y };
		auto rect = to_rectf(GetRect());
		if (!SDL_PointInRectFloat(&pt, &rect))
			return false;

		_fTargetScrollY -= toF(event.integer_y) * Constants::GUI::MouseScrollSpeed;
		_fTargetScrollY = std::clamp(_fTargetScrollY, 0.f, (float)_fMaxExtent);
		OnScroll();

		RefreshScrollBar();
		return true;
	}

	void ScrollPanel::OnAfterLayout()
	{
		if (_pSizer and not _children.empty())
		{
			Coord maxExtent = std::max(_fMaxExtent - GetHeight() + _topMargin + _bottomMargin, 0);
			_fScrollY = std::clamp(_fScrollY, 0.0f, toF(maxExtent));
			_fTargetScrollY = std::clamp(_fTargetScrollY, 0.0f, toF(maxExtent));

			// Move vertically
			for (auto& child : _children)
				child->SetY(child->GetY() - toI(_fScrollY) + _topMargin);
			
			RefreshScrollBar();
		}
	}

	void ScrollPanel::RefreshScrollBar()
	{
		if (_pScrollBar)
		{
			Coord maxExtent = std::max(_fMaxExtent - GetHeight() + _topMargin + _bottomMargin, 0);
			_pScrollBar->SetHeight(GetHeight());
			_pScrollBar->SetAbsolutePosition(GetAbsoluteX() + GetWidth() + _fScrollBarOffset, GetAbsoluteY()); // 16px offset??
			_pScrollBar->SetScroll(_fScrollY, maxExtent);
		}
	}
}