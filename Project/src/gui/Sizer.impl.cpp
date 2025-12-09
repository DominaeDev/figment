module GUI.Layout.Sizer;

import GUI.Layout.LayoutElement;

void Sizer::Add(LayoutElement* pControl, int proportion, int flags, int border)
{
	_items.push_back(LayoutInfo { pControl, proportion, flags, border, });
}

void Sizer::Remove(LayoutElement* pControl)
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

void Sizer::AddStretchSpacer()
{
	Add(nullptr, -1);
}