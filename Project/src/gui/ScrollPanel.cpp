#include <pch.h>
#include "gui/ScrollPanel.h"

namespace fig::gui
{
	ScrollPanel::ScrollPanel(Control* pParent) : Control(pParent)
	{
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
		if (!SDL_PointInRectFloat(&pt, &_rect))
			return false;

		_fScrollY = std::min(_fScrollY + toF(event.integer_y * Constants::GUI::MouseScrollSpeed * 1.5f), 0.0f);
		InvalidateLayout();
		return true;
	}

	void ScrollPanel::OnAfterLayout()
	{
		if (_pSizer)
		{
			float minY = std::numeric_limits<float>::max();
			float maxY = std::numeric_limits<float>::min();
			for (auto& child : _children)
			{
				auto& rect = child->GetRect();
				if (rect.y < minY)
					minY = rect.y;
				if (rect.y + rect.h > maxY)
					maxY = rect.y + rect.h;
			}

			_fMinExtent = minY;
			_fMaxExtent = maxY;

			for (auto& child : _children)
				child->SetY(child->GetY() + _fScrollY);
		}
	}
}