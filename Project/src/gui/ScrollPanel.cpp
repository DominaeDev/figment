#include <pch.h>
#include "gui/ScrollPanel.h"
#include "gui/GUIUtility.h"
#include "gui/VerticalScrollBar.h"
#include "app/AppState.h"
#include "app/AppSettings.h"

using namespace fig::io;

namespace fig::gui
{
	ScrollPanel::ScrollPanel(ControlPtr pParent, bool bScrollBar) : Control(pParent)
	{
		EnableCulling(true);
		EnableClipping(true);

		if (bScrollBar)
		{
			_pScrollBar = CreateControl<VerticalScrollBar>();
			_pScrollBar->SetWidth(16);
			_pScrollBar->SetHeight(GetHeight());
			_pScrollBar->SetX(GetWidth() - _pScrollBar->GetWidth());
			RemoveChild(_pScrollBar); // Unparented
		}
	}

	ScrollPanel::~ScrollPanel()
	{
		if (_pScrollBar)
			delete _pScrollBar;
	}

	static bool IsSmoothScrollingEnabled()
	{
		return Global::GetSettings().GetBool(AppSetting::Interface::SmoothScrolling);
	}

	void ScrollPanel::Render(fig::renderer_ptr pRenderer)
	{
		Control::Render(pRenderer);

		if (_pScrollBar and GetVisible() and not IsCulled())
			_pScrollBar->Render(pRenderer);
	}

	EventResult ScrollPanel::OnEvent(fig::event& event)
	{
		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			return HandleMouseWheel(event.wheel) ? EventResult::Handled : EventResult::Pass;
		}

		if (_pScrollBar)
			return _pScrollBar->OnEvent(event);

		return EventResult::Pass;
	}

	void ScrollPanel::OnUpdate(float fElapsed)
	{
		// Smooth scrolling
		if (IsSmoothScrollingEnabled())
		{
			if (_fScrollY != _fTargetScrollY)
			{
				_fScrollY += (_fTargetScrollY - _fScrollY) 
					* Constants::GUI::MouseScrollSmoothing 
					* std::min(fElapsed, 1.0f/30.0f); // Avoids stutter, overshooting at low frame rates

				if (std::abs(_fTargetScrollY - _fScrollY) < 0.5f)
					_fScrollY = _fTargetScrollY;
				InvalidateLayout();
			}
		}
		else
		{
			_fScrollY = _fTargetScrollY;
			InvalidateLayout();
		}

		if (_pScrollBar)
			return _pScrollBar->OnUpdate(fElapsed);
	}

	bool ScrollPanel::HandleMouseWheel(SDL_MouseWheelEvent event)
	{
		fig::pointf pt = { event.mouse_x, event.mouse_y };
		auto rect = to_rectf(GetRect());
		if (!SDL_PointInRectFloat(&pt, &rect))
			return false;

		_fTargetScrollY -= toF(event.integer_y) * Constants::GUI::MouseScrollSpeed;
		_fTargetScrollY = std::clamp(_fTargetScrollY, 0.f, (float)_maxExtent);
		
		OnScroll();

		RefreshScrollBar();
		return true;
	}

	fig::coord ScrollPanel::GetExtent() const
	{
		if (_children.empty())
			return 0;

		fig::coord minY = std::numeric_limits<fig::coord>::max();
		fig::coord maxY = std::numeric_limits<fig::coord>::min();

		for (auto& child : _children)
		{
			auto& rect = child->GetRect();
			minY = std::min(minY, rect.y);
			maxY = std::max(maxY, rect.y + rect.h);
		}
		return fig::coord { maxY - minY };
	}

	void ScrollPanel::OnAfterLayout()
	{
		if (_pSizer and not _children.empty())
		{
			_maxExtent = GetExtent();

			fig::coord extent = std::max(_maxExtent - GetHeight() + _topPadding + _bottomPadding, 0);
			_fScrollY = std::clamp(_fScrollY, 0.0f, toF(extent));
			_fTargetScrollY = std::clamp(_fTargetScrollY, 0.0f, toF(extent));

			// Move vertically
			for (auto& child : _children)
				child->SetY(child->GetY() - toI(_fScrollY) + _topPadding);
			
			_currentScrollY = toI(_fScrollY);
			RefreshScrollBar();
		}
	}

	void ScrollPanel::OnSize()
	{
		RefreshScrollBar();
	}

	void ScrollPanel::RefreshScrollBar()
	{
		if (_pScrollBar)
		{
			fig::coord maxExtent = std::max(_maxExtent - GetHeight() + _topPadding + _bottomPadding, 0);
			_pScrollBar->SetHeight(GetHeight());
			_pScrollBar->SetAbsolutePosition(GetAbsoluteX() + GetWidth() + _scrollBarOffset, GetAbsoluteY()); // 16px offset??
			_pScrollBar->SetScroll(*this, _fScrollY, maxExtent);
		}
	}

	void ScrollPanel::ScrollTo(float position, bool bSmooth) noexcept
	{
		_fTargetScrollY = std::clamp(position, 0.0f, (float)_maxExtent);
		if (!bSmooth)
			_fScrollY = _fTargetScrollY;
		
		OnScroll();
		InvalidateLayout();
		RefreshScrollBar();
	}

	void ScrollPanel::ResetScroll() noexcept
	{
		ScrollTo(0, false);
	}

}