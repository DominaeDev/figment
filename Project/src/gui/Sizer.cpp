#include <pch.h>
#include "gui/Sizer.h"
#include "gui/LayoutElement.h"
#include "gui/Area.h"
#include <cassert>
#include <cassert>

namespace fig::gui
{
	Sizer::~Sizer()
	{
		for (auto ppSizer : _items 
			| std::views::transform([](auto& li) { return std::get_if<Sizer*>(&li.pControl); }))
		{
			if (ppSizer)
				delete* ppSizer;
		}
	}

	void Sizer::Add(LayoutElement* pControl, int proportion, int flags, int border)
	{
		if (pControl)
		{
			_items.push_back(LayoutItem {
				.info = LayoutProperties { proportion, flags, border },
				.pControl = pControl,
			});
		}
	}

	void Sizer::AddSizer(Sizer* pSizer, int proportion, int flags, int border)
	{
		if (pSizer)
		{
			_items.push_back(LayoutItem {
				.info = LayoutProperties { proportion, flags | Sizer::Expand | Sizer::Fill, border },
				.pControl = pSizer,
			});
		}
	}

	void Sizer::Remove(LayoutElement* pControl)
	{
		auto it = std::find_if(_items.cbegin(), _items.cend(), [pControl](const LayoutItem & li) {
			if (auto pp = std::get_if<LayoutElement*>(&li.pControl); *pp and *pp == pControl)
				return true;
			return false;
		});

		if (it != _items.end())
			_items.erase(it);
	}

	void Sizer::RemoveSizer(Sizer* pSizer)
	{
		auto it = std::find_if(_items.cbegin(), _items.cend(), [pSizer](const LayoutItem & li) {
			if (auto pp = std::get_if<Sizer*>(&li.pControl); *pp and *pp == pSizer)
				return true;
			return false;
		});

		if (it != _items.end())
		{
			auto ppSizer = std::get_if<Sizer*>(&it->pControl);
			if (ppSizer)
				delete* ppSizer;
			_items.erase(it);
		}
	}

	void Sizer::Layout(const Rect& parentRect)
	{
		OnLayout(parentRect);

		// Update childen
		for (auto& li : _items)
		{
			if (auto pp = std::get_if<LayoutElement*>(&li.pControl))
				(*pp)->Layout(&li.rect);
			else if (auto pp = std::get_if<Sizer*>(&li.pControl))
				(*pp)->Layout(li.rect);
		}

	}

	void Sizer::Clear()
	{
		_items.clear();
	}

	void Sizer::AddSpacer(Coord size)
	{
		_items.emplace_back(LayoutItem {
			.info = LayoutProperties {
				.prop { 0 },
				.flags {},
				.border { 0 },
				.fixed { size },
			},
		});
	}

	void Sizer::AddStretchSpacer()
	{
		_items.emplace_back(LayoutItem {
			.info = LayoutProperties { 
				.prop { -1 },
				.flags {}, 
				.border { 0 },
			},
		});
	}

}