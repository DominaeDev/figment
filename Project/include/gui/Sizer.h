#pragma once

#include "Figment.h"
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
		enum Flag : int32_t {
			None = 0,
			Top = 1 << 0,
			Bottom = 1 << 1,
			Left = 1 << 2,
			Right = 1 << 3,
			Expand = 1 << 4,
			Fill = 1 << 5,
			FixedSize = 1 << 6, // if prop == 0
			FlexSize = 1 << 7,	// if prop < 0

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
		void Layout(const Rect& parentRect);
		virtual ~Sizer();

		void Add(LayoutElement* pControl, int32_t proportion = 0, int32_t flags = Flag::Default, int border = 0);
		void Add(Sizer* pControl, int32_t proportion = 0, int32_t flags = Flag::Default, int border = 0);
		void AddSpacer(Coord size);
		void AddStretchSpacer();
		void Remove(LayoutElement* pControl);
		void RemoveAll();
		void RemoveSizer(Sizer* pSizer);
		void Clear();

		template <typename T>
			requires std::derived_from<T, Sizer>
		void Add(fig::observer_ptr<T> pControl, int32_t proportion = 0, int32_t flags = Flag::Default, int border = 0) = delete;

		template <typename T>
			requires std::derived_from<T, Sizer>
		fig::observer_ptr<T> Add(int32_t proportion = 0, int32_t flags = Flag::Default, int border = 0)
		{
			auto pSizer = new T();
			Add(pSizer, proportion, flags, border);
			return fig::observer_ptr<T>(pSizer);
		}

	protected:
		struct LayoutProperties
		{
			int32_t prop { 0 };
			int32_t flags { Flag::None };
			int32_t border { 0 };

			inline int32_t leftBorder() const	{ return (flags & Flag::Left) != 0 ? border : 0; };
			inline int32_t rightBorder() const	{ return (flags & Flag::Right) != 0 ? border : 0; };
			inline int32_t topBorder() const	{ return (flags & Flag::Top) != 0 ? border : 0; };
			inline int32_t bottomBorder() const	{ return (flags & Flag::Bottom) != 0 ? border : 0; };
		};

		using LayoutElementPtr = LayoutElement*;
		using SizerPtr = Sizer*;
		using EmptyTarget = std::monostate;
		using SizerTarget = std::variant<EmptyTarget, LayoutElementPtr, SizerPtr>;

		struct SizerItem
		{
			LayoutProperties info {};
			SizerTarget target { EmptyTarget {} };
			Rect rect {};

			LayoutElementPtr GetControl() const
			{
				if (auto ppCtrl = std::get_if<LayoutElementPtr>(&target); ppCtrl)
					return *ppCtrl;
				return nullptr;
			};
		};

		fig::observer_ptr<LayoutElement> _pOwner;

		unsigned int GetCount() const { return static_cast<unsigned int>(_items.size()); }
		auto GetLayoutItems() noexcept
		{
			return _items
				| std::views::filter([](auto& it) { 
					if (auto ppCtrl = std::get_if<LayoutElementPtr>(&it.target); ppCtrl)
						return (*ppCtrl)->IsLayoutEnabled();
					return true;
				});
		}

		virtual void OnLayout(const Rect& rect) = 0;
		void Update(float fElapsed) override {};
	
	private:
		std::vector<SizerItem> _items;
		std::vector<std::unique_ptr<LayoutElement>> _dummies;
	};
}