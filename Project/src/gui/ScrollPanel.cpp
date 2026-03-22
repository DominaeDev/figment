#include <pch.h>
#include "gui/ScrollPanel.h"
#include "gui/GUIUtility.h"

using namespace fig::gui::util;

namespace fig::gui
{
	ScrollPanel::ScrollPanel(LayoutElement* pParent) : Control(pParent)
	{
		EnableCulling(true);
		EnableClipping(true);
	}

	bool ScrollPanel::OnEvent(Event& event)
	{
		if (event.type == SDL_EVENT_MOUSE_WHEEL)
		{
			return HandleMouseWheel(event.wheel);
		}
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
	}

	bool ScrollPanel::HandleMouseWheel(SDL_MouseWheelEvent event)
	{
		Pointf pt = { event.mouse_x, event.mouse_y };
		auto rect = to_rectf(GetRect());
		if (!SDL_PointInRectFloat(&pt, &rect))
			return false;

		_fTargetScrollY -= toF(event.integer_y) * Constants::GUI::MouseScrollSpeed;
		_fTargetScrollY = std::clamp(_fTargetScrollY, 0.f, (float)_maxExtent);
		return true;
	}

	void ScrollPanel::OnAfterLayout()
	{
		if (_pSizer and not _children.empty())
		{
			Coord maxExtent = std::max(_maxExtent - GetHeight() + _topMargin + _bottomMargin, 0);
			_fScrollY = std::clamp(_fScrollY, 0.0f, toF(maxExtent));
			_fTargetScrollY = std::clamp(_fTargetScrollY, 0.0f, toF(maxExtent));

			// Move vertically
			for (auto& child : _children)
				child->SetY(child->GetY() - toI(_fScrollY) + _topMargin);
		}
	}
}