#include "HorizontalSizer.h"
#include "Frame.h"

int CeilInt(float f)
{
	return (int)ceilf(f);
}

void HorizontalSizer::Layout(SDL_FRect parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int totalWidth = CeilInt(std::max(parentRect.w, 0.0f));
	int itemWidth = CeilInt((float)totalWidth / count);
	int remainingWidth = totalWidth;
	int totalProportion = 0;
	int numStretch = 0;
	for (auto& item : _items)
	{
		if (item.prop == 0)
			remainingWidth = CeilInt(std::max(remainingWidth - item.pFrame->GetWidth(), 0.0f));
		else if (item.prop > 0)
			totalProportion += item.prop;
		else
			numStretch++;
	}
	if (totalProportion == 0)
		totalProportion = 1;

	int x = 0;
	for (auto& item : _items)
	{
		auto& frame = *item.pFrame;
		auto& rect = frame.GetRect();
		int width = 0;
		if (item.prop == 0)
			width = CeilInt(frame.GetWidth());
		else if (item.prop > 0)
			width = CeilInt(item.prop * remainingWidth / (float)totalProportion);
		else
			width = CeilInt(remainingWidth / (float)numStretch);

		if (frame.GetMinSize().x > 0)
			width = CeilInt(std::max((float)width, frame.GetMinSize().x));
		if (frame.GetMaxSize().x > 0)
			width = CeilInt(std::min((float)width, frame.GetMaxSize().x));

		rect.x = parentRect.x + x;
		rect.y = parentRect.y;
		rect.w = (float)width;
		
		if ((item.flags & Flag::Expand) != 0)
			rect.h = parentRect.h;
		if ((item.flags & Flag::Up) != 0)
			rect.y = parentRect.y + item.border;
		if ((item.flags & Flag::Down) != 0)
			rect.h = std::min(rect.h, parentRect.h - (rect.y - parentRect.y) - item.border);
		if ((item.flags & Flag::Left) != 0)
			rect.x = (parentRect.x + x) + item.border;
		if ((item.flags & Flag::Right) != 0)
			rect.w = std::min(rect.w, width - (rect.x - (parentRect.x + x)) - item.border);

		x += width;
		frame.InvalidateLayout();
	}	
}