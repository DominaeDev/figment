export module Sizer:Sizer;

import Types;
export import Graphics;
export import IUpdateable;

import :LayoutElement;

export
{
	extern "C++" class Sizer : public IUpdateable
	{
	public:
		enum Flag : int {
			None = 0,
			Expand = 1 << 0,
			Top = 1 << 1,
			Bottom = 1 << 2,
			Left = 1 << 3,
			Right = 1 << 4,

			AlignLeft = 1 << 10,
			AlignRight = 1 << 11,
			AlignTop = 1 << 12,
			AlignBottom = 1 << 13,
			AlignCenterHorizontal = 1 << 14,
			AlignCenterVertical = 1 << 15,

			All = Top | Bottom | Left | Right,
			Default = None,
		};

	public:
		void Layout();
		void SetOwner(LayoutElement* pOwner);

		void Add(LayoutElement* pControl, int proportion = 0, int flags = Flag::Default, int border = 0);
		void AddStretchSpacer();
		void Remove(LayoutElement* pControl);
		void Clear();

	protected:
		struct LayoutInfo
		{
			LayoutElement* pControl;
			int prop = 0;
			int flags = Flag::None;
			int border = 0;
		};

		LayoutElement* _pOwner = nullptr;
		std::vector<LayoutInfo> _items;

		unsigned int GetCount() const { return static_cast<unsigned int>(_items.size()); }

		virtual void OnLayout(Rectf rect) = 0;
		void Update(float fDeltaTime) override {}
	};
}

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
