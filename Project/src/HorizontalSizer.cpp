#include "HorizontalSizer.h"

#include "Frame.h"

void HorizontalSizer::Layout(SDL_FRect parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	float totalWidth = std::max(parentRect.w, 0.0f);
	float itemWidth = totalWidth / count;
	float remainingWidth = totalWidth;
	int totalProportion = 0;
	int numStretch = 0;
	for (auto& item : _items)
	{
		if (item.prop == 0)
			remainingWidth = std::max(remainingWidth - item.pFrame->GetWidth(), 0.0f);
		else if (item.prop > 0)
			totalProportion += item.prop;
		else
			numStretch++;
	}
	if (totalProportion == 0)
		totalProportion = 1;

	float x = 0.0f;
	for (auto& item : _items)
	{
		auto& rect = item.pFrame->GetRect();
		float width = 0;
		if (item.prop == 0)
			width = rect.w; // No resize
		else if (item.prop > 0)
			width = item.prop * remainingWidth / (float)totalProportion;
		else
			width = remainingWidth / (float)numStretch;

		if (item.pFrame->GetMinSize().x > 0)
			width = std::max(width, item.pFrame->GetMinSize().x);
		if (item.pFrame->GetMaxSize().x > 0)
			width = std::max(width, item.pFrame->GetMaxSize().x);

		rect.x = parentRect.x + x;
		rect.y = parentRect.y;
		rect.w = width;

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
	}	
}