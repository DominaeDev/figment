#include <pch.h>
#include "gui/VerticalScrollSizer.h"
#include "gui/Control.h"

using namespace fig::gui;

void VerticalScrollSizer::SetOffset(Coord offset)
{
	Coord diff = offset - _offset;
	auto items = GetLayoutItems();

	for (auto it = items.begin(); it != items.end(); ++it)
	{
		auto pControl = (*it).pControl;
		if (pControl)
			pControl->SetY(pControl->GetY() + diff);
	}
	_offset = offset;
}

void VerticalScrollSizer::OnLayout(Rect rect)
{
	VerticalListSizer::OnLayout(rect);
	if (_offset == 0.0f)
		return;

	auto items = GetLayoutItems();

	for (auto it = items.begin(); it != items.end(); ++it)
	{
		auto pControl = (*it).pControl;
		if (pControl)
			pControl->SetY(pControl->GetY() + _offset);
	}
}