#pragma once

#include "gui/Control.h"
#include "gui/MouseEventHandler.h"
#include <ranges>

namespace fig::gui
{
	class DropListBase : public Control, public MouseEventHandler
	{
	public:
		int32_t Select(int32_t index, bool bSilent = false) noexcept;
		int32_t Select(fig::string_view label, bool bSilent = false) noexcept;
		int32_t GetSelectedIndex() const noexcept;

		void SetDelegate(ListItemSelectedDelegate fnDelegate);

	protected:
		DropListBase(ControlPtr pParent);

		void Clear() noexcept
		{
			_items.clear();
		}

		void AddItem(const fig::string_view& item) noexcept
		{
			_items.emplace_back(fig::string { item });
		}

		template <is_string_range T>
		void AddItems(const T& items) noexcept
		{
			for (auto& item : items)
				AddItem((fig::string)item);
		}

	protected:
		EventResult OnEvent(fig::event& event) override;
		void OnSize() override;
		void OnClicked() override;
		void OnEnabled(bool bEnabled) override;

		void RefreshText();

	protected:
		std::vector<fig::string> _items;
		int32_t _selectedIndex { -1 };
	
	private:
		fig::observer_ptr<StaticText> _pText;
		fig::observer_ptr<Image> _pArrow;
		int32_t _menuId {};
		ListItemSelectedDelegate _fnDelegate;
	};

	template <typename T>
	class DropListOfType : public DropListBase
	{
	public:
		DropListOfType(ControlPtr pParent) : DropListBase(pParent)
		{
		}

		void AddItem(const T& item) noexcept
			requires fig::is_string_like<T>
		{
			AddItem(static_cast<fig::string>(item), item);
		}

		template <std::ranges::range R>
			requires fig::is_string_like<T> and std::same_as<std::ranges::range_value_t<R>, T>
		void AddItems(const R& items) noexcept
		{
			for (auto& item : items)
				AddItem(static_cast<fig::string>(item), item);
		}

		void AddItem(fig::string_view label, T value) noexcept
		{
			DropListBase::AddItem(label);
			_values.push_back(std::move(value));
		}

		std::optional<T> GetValue() const noexcept
		{
			if (_selectedIndex >= 0 && _selectedIndex < _items.size())
				return _items[_selectedIndex].second;
			return std::nullopt;
		}

		int32_t SelectValue(const T& value, bool bSilent = false) noexcept
		{
			if (auto index = index_of(_values, value); index != fig::npos)
				return Select(static_cast<int32_t>(index), bSilent);
			else
				return Select(-1, bSilent);
		}

		void Clear()
		{
			DropListBase::Clear();
			_selectedIndex = -1;
		}

	private:
		std::vector<T> _values;
	};

	using DropList = DropListOfType<fig::string>;
}