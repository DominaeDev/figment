export module GUI.Layout.HorizontalSizer;

import Common;

import GUI.Layout.LayoutElement;

export
{
	class HorizontalSizer : public Sizer
	{
	protected:
		void OnLayout(Rectf rect) override;
	};
}

static int CeilInt(float f)
{
	return (int)std::ceilf(f);
}

void HorizontalSizer::OnLayout(Rectf parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int totalWidth = CeilInt(std::max(parentRect.w, 0.0f));
	int itemWidth = CeilInt((float)totalWidth / count);
	int remainingWidth = totalWidth;
	int totalProportion = 0;
	int numStretch = 0;
	for (auto& item : _items)
	{
		if (item.prop == 0 && item.pControl != nullptr)
			remainingWidth = CeilInt(std::max(remainingWidth - item.pControl->GetWidth(), 0.0f));
		else if (item.prop > 0)
			totalProportion += item.prop;
		else
			numStretch++;
	}
	if (totalProportion == 0)
		totalProportion = 1;

	int x = 0;
	for (auto& item : _items)
	{
		if (item.pControl == nullptr)
		{
			x += CeilInt(remainingWidth / (float)numStretch);
			continue;
		}

		auto& frame = *item.pControl;
		auto& rect = frame.GetRect();
		int width = 0;
		if (item.prop == 0)
			width = CeilInt(frame.GetWidth());
		else if (item.prop > 0)
			width = CeilInt(item.prop * remainingWidth / (float)totalProportion);
		else
			width = CeilInt(remainingWidth / (float)numStretch);

		if (frame.GetMinSize().x > 0)
			width = CeilInt(std::max(toF(width), frame.GetMinSize().x));
		if (frame.GetMaxSize().x > 0)
			width = CeilInt(std::min(toF(width), frame.GetMaxSize().x));

		Rectf borderRect {
			parentRect.x + x,
			parentRect.y,
			(float)width,
			parentRect.h,
		};

		if ((item.flags & Flag::Left) != 0)
		{
			borderRect.x += item.border;
			borderRect.w -= item.border;
		}
		if ((item.flags & Flag::Top) != 0)
		{
			borderRect.y += item.border;
			borderRect.h -= item.border;
		}
		if ((item.flags & Flag::Right) != 0)
			borderRect.w -= item.border;
		if ((item.flags & Flag::Bottom) != 0)
			borderRect.h -= item.border;

		rect.x = borderRect.x;
		rect.y = borderRect.y;
		rect.w = borderRect.w;

		if ((item.flags & Flag::Expand) != 0)
		{
			rect.h = borderRect.h;
		}
		else
		{
			if ((item.flags & Flag::AlignLeft) != 0)
				rect.x = borderRect.x;
			else if ((item.flags & Flag::AlignCenterHorizontal) != 0)
				rect.x = borderRect.x + (borderRect.w - rect.w) / 2;
			else if ((item.flags & Flag::AlignRight) != 0)
				rect.x = borderRect.x + borderRect.w - rect.w;
			if ((item.flags & Flag::AlignTop) != 0)
				rect.y = borderRect.y;
			else if ((item.flags & Flag::AlignCenterVertical) != 0)
				rect.y = borderRect.y + (borderRect.h - rect.h) / 2;
			else if ((item.flags & Flag::AlignBottom) != 0)
				rect.y = borderRect.y + borderRect.h - rect.h;
		}

		frame.SetPosition(rect.x - parentRect.x, rect.y - parentRect.y);
		frame.SetSize(rect.w, rect.h);

		x += width;
	}
}