#include "gui/VerticalSizer.h"
#include "gui/Control.h"

static int CeilInt(float f)
{
	return (int)ceilf(f);
}

void VerticalSizer::OnLayout(SDL_FRect parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int totalHeight = CeilInt(std::max(parentRect.h, 0.0f));
	int itemHeight= CeilInt((float)totalHeight/ count);
	int remainingHeight = totalHeight;
	int totalProportion = 0;
	int numStretch = 0;
	for (auto& item : _items)
	{
		if (item.prop == 0 && item.pControl != nullptr)
			remainingHeight = CeilInt(std::max(remainingHeight - item.pControl->GetHeight(), 0.0f));
		else if (item.prop > 0)
			totalProportion += item.prop;
		else
			numStretch++;
	}
	if (totalProportion == 0)
		totalProportion = 1;

	int y = 0;
	for (auto& item : _items)
	{
		if (item.pControl == nullptr)
		{
			y += CeilInt(remainingHeight / (float)numStretch);
			continue;
		}

		auto& frame = *item.pControl;
		auto& rect = frame.GetRect();
		int height = 0;
		if (item.prop == 0)
			height = CeilInt(frame.GetHeight());
		else if (item.prop > 0)
			height = CeilInt(item.prop * remainingHeight / (float)totalProportion);
		else
			height = CeilInt(remainingHeight / (float)numStretch);

		if (frame.GetMinSize().y > 0)
			height = CeilInt(std::max((float)height, frame.GetMinSize().y));
		if (frame.GetMaxSize().y > 0)
			height = CeilInt(std::min((float)height, frame.GetMaxSize().y));

		SDL_FRect borderRect {
			parentRect.x,
			parentRect.y + y,
			parentRect.w,
			(float)height,
		};

		if ((item.flags & Flag::Left) != 0)
		{
			borderRect.x += item.border;
			borderRect.w -= item.border;
		}
		if ((item.flags & Flag::Top) != 0)
		{
			borderRect.y += item.border;
			borderRect.h -= item.border;
		}
		if ((item.flags & Flag::Right) != 0)
			borderRect.w -= item.border;
		if ((item.flags & Flag::Bottom) != 0)
			borderRect.h -= item.border;

		rect.x = borderRect.x;
		rect.y = borderRect.y;
		rect.h = borderRect.h;

		if ((item.flags & Flag::Expand) != 0)
		{
			rect.w = borderRect.w;
		}
		else
		{
			if ((item.flags & Flag::AlignTop) != 0)
				rect.y = borderRect.y;
			else if ((item.flags & Flag::AlignCenterVertical) != 0)
				rect.y = borderRect.y + (borderRect.h - rect.h) / 2;
			else if ((item.flags & Flag::AlignBottom) != 0)
				rect.y = borderRect.y + borderRect.h - rect.h;
			if ((item.flags & Flag::AlignLeft) != 0)
				rect.x = borderRect.x;
			else if ((item.flags & Flag::AlignCenterHorizontal) != 0)
				rect.x = borderRect.x + (borderRect.w - rect.w) / 2;
			else if ((item.flags & Flag::AlignRight) != 0)
				rect.x = borderRect.x + borderRect.w - rect.w;
		}

		y += height;

		frame.SetPosition(rect.x - parentRect.x, rect.y - parentRect.y);
		frame.SetSize(rect.w, rect.h);
	}	
}