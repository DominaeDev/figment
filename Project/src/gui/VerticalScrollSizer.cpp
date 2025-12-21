#include <pch.h>
#include "gui/VerticalScrollSizer.h"
#include "gui/Control.h"

using namespace fig::gui;

void VerticalScrollSizer::SetOffset(float offset)
{
	float diff = offset - _offset;
	for (auto it = _items.begin(); it != _items.end(); ++it)
	{
		auto pControl = (*it).pControl;
		if (pControl)
			pControl->SetY(pControl->GetY() + diff);
	}
	_offset = offset;
}

void VerticalScrollSizer::OnLayout(Rectf rect)
{
	VerticalListSizer::OnLayout(rect);
	if (_offset == 0.0f)
		return;

	for (auto it = _items.begin(); it != _items.end(); ++it)
	{
		auto pControl = (*it).pControl;
		if (pControl)
			pControl->SetY(pControl->GetY() + _offset);
	}
}