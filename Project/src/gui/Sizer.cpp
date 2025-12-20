#include "gui/Sizer.h"
#include "gui/LayoutElement.h"

using namespace fig::gui;

void Sizer::Add(Control* pControl, int proportion, int flags, int border)
{
	_items.push_back(LayoutInfo { pControl, proportion, flags, border, });
}

void Sizer::Remove(Control* pControl)
{
	auto it = std::find_if(_items.cbegin(), _items.cend(), [pControl](const LayoutInfo& li) {
		return li.pControl == pControl;
	});

	if (it != _items.end())
		_items.erase(it);
}

void Sizer::Layout()
{
	if (_pOwner)
		OnLayout(_pOwner->GetRect());
}

void Sizer::Clear()
{
	_items.clear();
}

void Sizer::SetOwner(LayoutElement* pOwner)
{
	_pOwner = pOwner;
}