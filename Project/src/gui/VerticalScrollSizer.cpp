#include "gui/VerticalScrollSizer.h"
#include "gui/Control.h"

void VerticalScrollSizer::SetOffset(float offset)
{
	float diff = offset - _offset;
	for (auto it = std::begin(_items); it != std::end(_items); ++it)
	{
		auto pControl = (*it).pControl;
		if (pControl)
			pControl->SetY(pControl->GetY() + diff);
	}
	_offset = offset;
}

void VerticalScrollSizer::OnLayout(SDL_FRect rect)
{
	VerticalListSizer::OnLayout(rect);
	if (_offset == 0.0f)
		return;

	for (auto it = std::begin(_items); it != std::end(_items); ++it)
	{
		auto pControl = (*it).pControl;
		if (pControl)
			pControl->SetY(pControl->GetY() + _offset);
	}
}