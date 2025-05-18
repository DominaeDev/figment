#include "VerticalListSizer.h"
#include "Control.h"

static int CeilInt(float f)
{
	return (int)ceilf(f);
}

void VerticalListSizer::OnLayout(SDL_FRect parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int y = 0;
	for (auto it = std::rbegin(_items); it != std::rend(_items); ++it)
	{
		auto& item = *it;
		if (item.pFrame == nullptr)
			continue;

		auto& frame = *item.pFrame;
		auto& rect = frame.GetRect();
		int height = CeilInt(frame.GetHeight());

		rect.x = parentRect.x;

		if ((item.flags & Flag::Expand) != 0)
			rect.w = parentRect.w;

		rect.y = parentRect.y + parentRect.h - y - rect.h - _marginBottom;

		frame.SetPosition(rect.x - parentRect.x, rect.y - parentRect.y);
		frame.SetSize(rect.w, rect.h);

		y += height + _spacing;
	}	
}