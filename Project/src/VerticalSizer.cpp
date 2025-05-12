#include "VerticalSizer.h"
#include "Control.h"

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
		if (item.prop == 0)
			remainingHeight = CeilInt(std::max(remainingHeight - item.pFrame->GetHeight(), 0.0f));
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
		auto& frame = *item.pFrame;
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

		rect.x = parentRect.x;
		rect.y = parentRect.y + y;
		rect.h = (float)height;
		
		if ((item.flags & Flag::Expand) != 0)
			rect.w = parentRect.w;

		if ((item.flags & Flag::Left) != 0)
			rect.x = parentRect.x + item.border;
		if ((item.flags & Flag::Right) != 0)
			rect.w = std::min(rect.w, parentRect.w - (rect.x - parentRect.x) - item.border);
		if ((item.flags & Flag::Up) != 0)
			rect.y = (parentRect.y + y) + item.border;
		if ((item.flags & Flag::Down) != 0)
			rect.h = std::min(rect.h, height - (rect.y - (parentRect.y + y)) - item.border);

		y += height;

		frame.InvalidateLayout();
	}	
}