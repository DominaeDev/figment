#include <pch.h>
#include "gui/ScrollPanel.h"

namespace fig::gui
{
	ScrollPanel::ScrollPanel(Control* pParent) : Control(pParent)
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
		if (!SDL_PointInRectFloat(&pt, &_rect))
			return false;

		_fScrollY -= toF(event.integer_y * Constants::GUI::MouseScrollSpeed * 1.5f);
		
		InvalidateLayout();
		return true;
	}

	void ScrollPanel::OnAfterLayout()
	{
		if (_pSizer and not _children.empty())
		{
			if constexpr (Disabled) // Doesn't work as expected
			{
				float minY = std::numeric_limits<float>::max();
				float maxY = std::numeric_limits<float>::min();
				for (auto& child : _children)
				{
					auto& pos = child->GetPosition();
					auto& size = child->GetSize();
					minY = std::min(minY, pos.y);
					maxY = std::max(maxY, pos.y + size.y);
				}

				_fMaxExtent = maxY - minY;
			}

			float maxExtent = std::max(_fMaxExtent - GetHeight() + _fBottomMargin, 0.0f);
			_fScrollY = std::clamp(_fScrollY, 0.0f, maxExtent);

			// Move vertically
			for (auto& child : _children)
				child->SetY(child->GetY() - _fScrollY);
		}
	}
}