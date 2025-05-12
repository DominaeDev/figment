#include "Sizer.h"

void Sizer::Add(Control* pFrame, int proportion, int flags, int border)
{
	_items.push_back(LayoutInfo { pFrame, proportion, flags, border, });
}

void Sizer::Remove(Control* pFrame)
{
	auto it = std::find_if(std::cbegin(_items), std::cend(_items), [pFrame](const LayoutInfo& li) {
		return li.pFrame == pFrame;
	});

	if (it != std::end(_items))
		_items.erase(it);
}

void Sizer::Layout()
{
	LayoutElement::Layout();
	if (_pParent)
		OnLayout(_pParent->GetRect());
}

void Sizer::Reset()
{
	_items.clear();
}
