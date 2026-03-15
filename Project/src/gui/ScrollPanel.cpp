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

	bool ScrollPanel::HandleMouseWheel(SDL_MouseWheelEvent event)
	{
		Pointf pt = { event.mouse_x, event.mouse_y };
		auto rect = to_rectf(GetRect());
		if (!SDL_PointInRectFloat(&pt, &rect))
			return false;

		_fScrollY -= toF(event.integer_y) * Constants::GUI::MouseScrollSpeed * 1.5f;
		
		InvalidateLayout();
		return true;
	}

	void ScrollPanel::OnAfterLayout()
	{
		if (_pSizer and not _children.empty())
		{
			Coord maxExtent = std::max(_maxExtent - GetHeight() + _topMargin + _bottomMargin, 0);
			_fScrollY = std::clamp(_fScrollY, 0.0f, toF(maxExtent));

			// Move vertically
			for (auto& child : _children)
				child->SetY(child->GetY() - toI(_fScrollY) + _topMargin);
		}
	}
}