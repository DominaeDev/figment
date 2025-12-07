export module Sizer:VerticalListSizer;

import Types;
import :Sizer;

export
{
	class VerticalListSizer : public Sizer
	{
	public:
		void SetSpacing(int spacing) { _spacing = spacing; }
		void SetBottomMargin(int margin) { _marginBottom = margin; }

		float GetListHeight() const { return _totalListHeight; }

	protected:
		virtual void OnLayout(Rectf rect) override;

		float _totalListHeight;
		int _marginBottom = 0;
		int _spacing = 0;
	};
}

int CeilInt(float f)
{
	return (int)std::ceilf(f);
}

void VerticalListSizer::OnLayout(Rectf parentRect)
{
	auto count = GetCount();
	if (count == 0)
		return;

	int y = 0;
	for (auto it = _items.rbegin(); it != _items.rend(); ++it)
	{
		auto& item = *it;
		if (item.pControl == nullptr)
			continue;

		auto& frame = *item.pControl;
		auto& rect = frame.GetRect();
		int height = CeilInt(frame.GetHeight());

		rect.x = parentRect.x;

		if ((item.flags & Flag::Expand) != 0)
			rect.w = parentRect.w;

		rect.y = parentRect.y + parentRect.h - y - rect.h - _marginBottom;

		frame.SetPosition(rect.x - parentRect.x, rect.y - parentRect.y);
		frame.SetSize(rect.w, rect.h);

		y += height + _spacing;
	}
	_totalListHeight = toF(y - _spacing);
}