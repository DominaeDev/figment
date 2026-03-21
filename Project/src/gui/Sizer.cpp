#include <pch.h>
#include "gui/Sizer.h"
#include "gui/LayoutElement.h"
#include "gui/Area.h"
#include <cassert>

namespace fig::gui
{
	class LayoutDummy : public LayoutElement {};

	void Sizer::Add(LayoutElement* pControl, int proportion, int flags, int border)
	{
		_items.push_back(LayoutInfo { pControl, proportion, flags, border });
	}

	Control* Sizer::Add(Sizer* pSizer, Control* pParent, int proportion, int flags, int border)
	{
		auto pControl = new Area(pParent);
		pControl->SetSizer(pSizer);
		_items.push_back(LayoutInfo { pControl, proportion, flags, border });
		return pControl;
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

	LayoutElement* Sizer::MakeDummy()
	{
		auto pDummy = std::make_unique<LayoutDummy>();
		_dummies.emplace_back(std::move(pDummy));
		return _dummies.back().get();
	}

	void Sizer::AddSpacer(Coord size)
	{
		auto dummy = MakeDummy();
		dummy->SetSize(size, size);
		_items.push_back(LayoutInfo { dummy, 0, {}, 0 });
	}

	void Sizer::AddStretchSpacer()
	{
		_items.push_back(LayoutInfo { MakeDummy(), -1, {}, 0});
	}

}