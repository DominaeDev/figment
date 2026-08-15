#include <pch.h>
#include "gui/Sizer.h"
#include "gui/LayoutElement.h"
#include "gui/Area.h"
#include <cassert>

namespace fig::gui
{
	Sizer::~Sizer()
	{
		for (auto ppSizer : _items 
			| std::views::transform([](auto& li) { return std::get_if<Sizer*>(&li.target); }))
		{
			if (ppSizer)
				delete* ppSizer;
		}
	}

	SizerItem& Sizer::Add(LayoutElement* pControl, int32_t proportion, int32_t flags, int border)
	{
		_items.emplace_back(SizerItem {
			.info = LayoutProperties { 
				.prop = proportion, 
				.flags = flags, 
				.border = border
			},
			.target = pControl,
		});
		return _items.back();
	}

	SizerItem& Sizer::Add(Sizer* pSizer, int32_t proportion, int32_t flags, int border)
	{
		_items.emplace_back(SizerItem {
			.info = LayoutProperties { 
				.prop = proportion, 
				.flags = flags | SizerFlag::Fill,
				.border = border 
			},
			.target = pSizer,
		});
		return _items.back();
	}

	SizerItem& Sizer::AddSpacer(fig::coord size)
	{
		_items.emplace_back(SizerItem {
			.info = LayoutProperties {
				.prop { 0 },
				.flags { SizerFlag::FixedSize },
				.border { size },
			},
		});
		return _items.back();
	}

	SizerItem& Sizer::AddStretchSpacer()
	{
		_items.emplace_back(SizerItem {
			.info = LayoutProperties {
				.prop { -1 },
				.flags {},
				.border { 0 },
			},
		});
		return _items.back();
	}

	void Sizer::Remove(LayoutElement* pControl)
	{
		auto it = std::find_if(_items.cbegin(), _items.cend(), [pControl](const SizerItem & li) {
			if (auto pp = std::get_if<LayoutElement*>(&li.target); pp and *pp == pControl)
				return true;
			return false;
		});

		if (it != _items.end())
			_items.erase(it);
	}

	void Sizer::Remove(Sizer* pSizer)
	{
		auto it = std::find_if(_items.cbegin(), _items.cend(), [pSizer](const SizerItem & li) {
			if (auto pp = std::get_if<Sizer*>(&li.target); pp and *pp == pSizer)
				return true;
			return false;
		});

		if (it != _items.end())
		{
			auto ppSizer = std::get_if<Sizer*>(&it->target);
			if (ppSizer)
				delete* ppSizer;
			_items.erase(it);
		}
	}

	void Sizer::RemoveAll()
	{
		for (auto it = _items.begin(); it != _items.end(); ++it)
		{
			auto ppSizer = std::get_if<Sizer*>(&it->target);
			if (ppSizer)
				delete* ppSizer;
		}
		_items.clear();
	}

	void Sizer::Layout(const fig::rect& parentRect)
	{
		OnPreLayout(parentRect);

		OnLayout(parentRect);

		for (auto& li : _items)
		{
			if (auto pp = std::get_if<LayoutElement*>(&li.target))
				(*pp)->Layout();
			else if (auto pp = std::get_if<Sizer*>(&li.target))
				(*pp)->Layout(li.rect);
		}
	}

	void Sizer::Clear()
	{
		_items.clear();
	}

	void Sizer::ApplyBorder(fig::rect& rect, const SizerItem& item)
	{
		// Apply border
		if (item.info.IsSet(SizerFlag::Left))
		{
			rect.x += item.info.border;
			rect.w -= item.info.border;
		}
		if (item.info.IsSet(SizerFlag::Top))
		{
			rect.y += item.info.border;
			rect.h -= item.info.border;
		}
		if (item.info.IsSet(SizerFlag::Right))
		{
			rect.w -= item.info.border;
		}
		if (item.info.IsSet(SizerFlag::Bottom))
		{
			rect.h -= item.info.border;
		}
	}

	void Sizer::ClampRect(fig::rect& rect, const SizerItem& item)
	{
		if (auto pControl = item.GetControl())
		{
			auto minSize = pControl->GetMinSize();
			auto maxSize = pControl->GetMaxSize();
			if (maxSize.x > 0)
				rect.w = std::min(rect.w, maxSize.x);
			if (maxSize.y > 0)
				rect.h = std::min(rect.h, maxSize.y);
			if (minSize.x > 0)
				rect.w = std::max(rect.w, minSize.x);
			if (minSize.y > 0)
				rect.h = std::max(rect.h, minSize.y);
		}
	}

	void Sizer::AlignRect(fig::rect& rect, const fig::rect& allocatedRect, const SizerItem& item)
	{
		if (item.info.IsSet(SizerFlag::AlignTop))
			rect.y = allocatedRect.y;
		else if (item.info.IsSet(SizerFlag::AlignCenterVertical))
			rect.y = allocatedRect.y + (allocatedRect.h - rect.h) / 2;
		else if (item.info.IsSet(SizerFlag::AlignBottom))
			rect.y = allocatedRect.y + allocatedRect.h - rect.h;
		if (item.info.IsSet(SizerFlag::AlignLeft))
			rect.x = allocatedRect.x;
		else if (item.info.IsSet(SizerFlag::AlignCenterHorizontal))
			rect.x = allocatedRect.x + (allocatedRect.w - rect.w) / 2;
		else if (item.info.IsSet(SizerFlag::AlignRight))
			rect.x = allocatedRect.x + allocatedRect.w - rect.w;
	}
}