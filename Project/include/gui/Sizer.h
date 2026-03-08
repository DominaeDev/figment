#pragma once

#include "Types.h"
#include "gui/GUITypes.h"
#include "IUpdateable.h"
#include <ranges>

namespace fig::gui
{
	class Control;
	class LayoutElement;

	class Sizer : public IUpdateable
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
		Control* Add(Sizer* pControl, Control* pParent, int proportion = 0, int flags = Flag::Default, int border = 0);
		void AddStretchSpacer();
		void Remove(Control* pControl);
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

		unsigned int GetCount() const { return static_cast<unsigned int>(_items.size()); }
		auto GetLayoutItems() const noexcept
		{
			return _items
				| std::views::filter([](auto& i) { return (bool)i.pControl and i.pControl->IsLayoutEnabled(); });
		}

		virtual void OnLayout(Rectf rect) = 0;
		void Update(float fDeltaTime) override {};

	private:
		std::vector<LayoutInfo> _items;
	};
}