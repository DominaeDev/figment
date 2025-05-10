#include "HorizontalSizer.h"
#include "Frame.h"

void HorizontalSizer::Layout(SDL_FRect parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int totalWidth = (int)ceilf(std::max(parentRect.w, 0.0f));
	int itemWidth = (int)ceilf((float)totalWidth / count);
	int remainingWidth = totalWidth;
	int totalProportion = 0;
	int numStretch = 0;
	for (auto& item : _items)
	{
		if (item.prop == 0)
			remainingWidth = std::max(remainingWidth - (int)item.pFrame->GetPreferredSize().x, 0);
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
		auto& rect = item.pFrame->GetRect();
		int width = 0;
		if (item.prop == 0)
			width = (int)item.pFrame->GetPreferredSize().x;
		else if (item.prop > 0)
			width = (int)(item.prop * remainingWidth / (float)totalProportion);
		else
			width = (int)(remainingWidth / (float)numStretch);

		if (item.pFrame->GetMinSize().x > 0)
			width = std::max(width, (int)item.pFrame->GetMinSize().x);
		if (item.pFrame->GetMaxSize().x > 0)
			width = std::min(width, (int)item.pFrame->GetMaxSize().x);

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
	}	
}