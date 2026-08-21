#pragma once

#include "gui/TextInput.h"

namespace fig::gui
{
	class ComboBox : public TextInput, public MouseEventHandler
	{
	public:
		ComboBox(ControlPtr pParent);

		int32_t Select(int32_t index, bool bSilent = false) noexcept;
		int32_t SelectValue(fig::string_view value, bool bSilent = false) noexcept;
		int32_t GetSelectedIndex() const noexcept;

		void AddItem(const fig::string_view& item) noexcept;
		template <is_string_range T>
		void AddItems(const T& items) noexcept
		{
			for (auto& item : items)
				AddItem((fig::string)item);
		}
		void Clear() noexcept;

		void SetDelegate(ListItemSelectedDelegate fnDelegate);

	protected:
		void OnEnabled(bool bEnabled) override;
		void OnSize() override;
		void OnText(fig::string_view text) override;
		EventResult OnEvent(fig::event& event) override;

		std::vector<fig::string> _items;
		int32_t _selectedIndex { -1 };
		ListItemSelectedDelegate _fnDelegate;

	private:
		void RefreshText();
		void OnClicked();

		fig::observer_ptr<Image> _pArrow;
		int32_t _menuId {};
		bool _bIgnoreText { false };
	};
}