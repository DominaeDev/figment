#pragma once

#include "Figment.h"
#include "gui/GUITypes.h"
#include <ranges>

namespace fig::gui
{
	class Control;
	class LayoutElement;

	enum SizerFlag : int32_t 
	{
		None = 0,
		Top = 1 << 0,
		Bottom = 1 << 1,
		Left = 1 << 2,
		Right = 1 << 3,
		Expand = 1 << 4,
		Fill = 1 << 5,
		FixedSize = 1 << 6, // if prop == 0
		Greedy = 1 << 7,	// if prop < 0

		AlignLeft = 1 << 10,
		AlignRight = 1 << 11,
		AlignTop = 1 << 12,
		AlignBottom = 1 << 13,
		AlignCenterHorizontal = 1 << 14,
		AlignCenterVertical = 1 << 15,

		All = Top | Bottom | Left | Right,
	};

	struct LayoutProperties
	{
		int32_t prop { 0 };
		int32_t flags { 0 };
		fig::coord border { 0 };

		constexpr inline bool IsSet(int32_t flag) const { return (flags & flag); }
		constexpr inline bool IsSet(SizerFlag flag) const { return (flags & static_cast<int32_t>(flag)); }
	};

	struct SizerItem
	{
		using SizerTarget = std::variant<std::monostate, LayoutElement*, Sizer*>;
		using EmptyTarget = std::monostate;

		LayoutProperties info {};
		SizerTarget target { EmptyTarget {} };
		fig::rect rect {};

		LayoutElement* GetControl() const
		{
			if (auto ppCtrl = std::get_if<LayoutElement*>(&target); ppCtrl)
				return *ppCtrl;
			return nullptr;
		};

		Sizer* GetSizer() const
		{
			if (auto ppSizer = std::get_if<Sizer*>(&target); ppSizer)
				return *ppSizer;
			return nullptr;
		};

		template <typename T>
			requires std::derived_from<T, Control>
		T* GetControl() const
		{
			if (auto ppCtrl = std::get_if<LayoutElement*>(&target); ppCtrl)
				return dynamic_cast<T*>(*ppCtrl);
			return nullptr;
		};

		constexpr inline fig::coord GetLeftBorder() const { return info.IsSet(SizerFlag::Left) ? info.border : 0; };
		constexpr inline fig::coord GetRightBorder() const { return info.IsSet(SizerFlag::Right) ? info.border : 0; };
		constexpr inline fig::coord GetTopBorder() const { return info.IsSet(SizerFlag::Top) ? info.border : 0; };
		constexpr inline fig::coord GetBottomBorder() const { return info.IsSet(SizerFlag::Bottom) ? info.border : 0; };
	};

	class Sizer
	{
	public:
		virtual ~Sizer();

		SizerItem& Add(LayoutElement* pControl, int32_t proportion = 0, int32_t flags = 0, int border = 0);
		SizerItem& Add(Sizer* pControl, int32_t proportion = 0, int32_t flags = 0, int border = 0);
		SizerItem& AddSpacer(fig::coord size);
		SizerItem& AddStretchSpacer();
		void Remove(LayoutElement* pControl);
		void Remove(Sizer* pSizer);
		void RemoveAll();
		void Clear();

		template <typename T>
			requires std::derived_from<T, Sizer>
		void Add(fig::observer_ptr<T> pControl, int32_t proportion = 0, int32_t flags = 0, int border = 0) = delete;

		template <typename T>
			requires std::derived_from<T, Sizer>
		fig::observer_ptr<T> Add(int32_t proportion = 0, int32_t flags = 0, int border = 0)
		{
			auto pSizer = new T();
			Add(pSizer, proportion, flags, border);
			return fig::observer_ptr<T>(pSizer);
		}

		void Layout(const fig::rect& parentRect);

	protected:
		void PreLayout(const fig::rect& rect);

		unsigned int GetCount() const { return static_cast<unsigned int>(_items.size()); }
		auto GetLayoutItems() noexcept
		{
			return _items
				| std::views::filter([](auto& it) { 
					if (auto ppCtrl = std::get_if<LayoutElement*>(&it.target); ppCtrl)
						return (*ppCtrl)->IsLayoutEnabled();
					return true;
				});
		}

		auto GetLayoutItems() const noexcept
		{
			return _items
				| std::views::filter([](auto& it) {
					if (auto ppCtrl = std::get_if<LayoutElement*>(&it.target); ppCtrl)
						return (*ppCtrl)->IsLayoutEnabled();
					return true;
				});
		}

		auto GetSizerItems() noexcept
		{
			return _items
				| std::views::filter([](auto& it) { return std::holds_alternative<Sizer*>(it.target); });
		}

		auto GetSizerItems() const noexcept
		{
			return _items
				| std::views::filter([](auto& it) { return std::holds_alternative<Sizer*>(it.target); });
		}

		virtual void OnLayout(const fig::rect& rect) = 0;
		virtual void OnLayoutItem(fig::rect& itemRect, SizerItem& item) {};
		
		void ApplyBorder(fig::rect& rect, const SizerItem& item);
		void ClampRect(fig::rect& rect, const SizerItem& item);
		void AlignRect(fig::rect& rect, const fig::rect& allocatedRect, const SizerItem& item);

		LayoutElement* _pOwner;
	private:
		std::vector<SizerItem> _items;
		std::vector<std::unique_ptr<LayoutElement>> _dummies;
	};

	class SizerWithExtents : public Sizer
	{
	public:
		virtual fig::point GetExtents() const = 0;
	};

}