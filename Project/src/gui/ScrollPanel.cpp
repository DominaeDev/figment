#include <pch.h>
#include "gui/ScrollPanel.h"

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
		if (!SDL_PointInRectFloat(&pt, &GetRect()))
			return false;

		_fScrollY -= toF(event.integer_y * Constants::GUI::MouseScrollSpeed * 1.5f);
		
		InvalidateLayout();
		return true;
	}

	void ScrollPanel::OnAfterLayout()
	{
		if (_pSizer and not _children.empty())
		{
			float maxExtent = std::max(_fMaxExtent - GetHeight() + _fTopMargin + _fBottomMargin, 0.0f);
			_fScrollY = std::clamp(_fScrollY, 0.0f, maxExtent);

			// Move vertically
			for (auto& child : _children)
				child->SetY(child->GetY() - _fScrollY + _fTopMargin);
		}
	}
}