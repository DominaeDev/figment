#include <pch.h>
#include "gui/ComboBox.h"
#include "gui/AppResources.h"
#include "gui/CustomRenderers.h"
#include "gui/Menu.h"

namespace fig::gui
{
	ComboBox::ComboBox(ControlPtr pParent) : TextInput(pParent, FontFace::Default, Constants::GUI::DefaultFontSize), MouseEventHandler(this)
	{
		_pArrow = CreateControl<Image>(Resource::ICON_DROPLIST_ARROW);
		_pArrow->SetForegroundColor(Color::Icon);

		SetMargins(8, 4, 38, 6);
		SetSize(300, 32);

		auto pTextBoxBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetColor(Color::TextBoxBackground);

		auto pTextBoxBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetColor(Color::LineColor);
	}

	void ComboBox::SetDelegate(ListItemSelectedDelegate fnDelegate)
	{
		_fnDelegate = fnDelegate;
	}

	void ComboBox::Clear() noexcept
	{
		_items.clear();
	}

	void ComboBox::AddItem(const fig::string_view& item) noexcept
	{
		_items.emplace_back(fig::string { item });
	}

	int32_t ComboBox::GetSelectedIndex() const noexcept
	{
		return _selectedIndex;
	}

	void ComboBox::OnSize()
	{
		_pArrow->SetX(GetWidth() - 4 - _pArrow->GetWidth());
		_pArrow->CenterVertically();

		MouseEventHandler::SetClickableRegion(fig::rect { GetWidth() - 38, 0, 38, GetHeight() });
	}

	void ComboBox::OnText(fig::string_view text)
	{
		if (_bIgnoreText)
			return;

		int32_t prevSelectedIndex = _selectedIndex;
		auto trimmed = trim(text);
		if (auto itFind = std::find_if(_items.cbegin(), _items.cend(), [trimmed](auto&& i) { return equals(trimmed, i, true); }); itFind != _items.cend())
			_selectedIndex = static_cast<int32_t>(std::distance(_items.cbegin(), itFind));		
		else
			_selectedIndex = -1;

		if (prevSelectedIndex != _selectedIndex and _fnDelegate)
			_fnDelegate(_selectedIndex);
	}

	void ComboBox::OnEnabled(bool bEnabled)
	{
		TextInput::OnEnabled(bEnabled);
		GetBackgroundRenderer()->SetColor(bEnabled ? Color::White : Color::DisabledBackground);
		GetBorderRenderer()->SetColor(bEnabled ? Color::LineColor : Color::DisabledLineColor);
		_pArrow->SetForegroundColor(bEnabled ? Color::Icon : Color::DisabledForeground);

		MouseEventHandler::Enable(bEnabled);
	}

	EventResult ComboBox::OnEvent(fig::event& event)
	{
		if (auto result = MouseEventHandler::HandleMouseEvents(event); result == EventResult::Handled)
			return result;

		return TextInput::OnEvent(event);
	}

	int32_t ComboBox::Select(int32_t index, bool bSilent) noexcept
	{
		if (index >= 0 && index < _items.size())
			_selectedIndex = index;
		else
			_selectedIndex = -1;

		if (_fnDelegate and not bSilent)
			_fnDelegate(_selectedIndex);

		RefreshText();
		return _selectedIndex;
	}

	int32_t ComboBox::SelectValue(fig::string_view text, bool bSilent) noexcept
	{
		auto trimmed = trim(text);
		if (auto itFind = std::find_if(_items.cbegin(), _items.cend(), [trimmed](auto&& i) { return equals(trimmed, i, true); }); itFind != _items.cend())
			_selectedIndex = static_cast<int32_t>(std::distance(_items.cbegin(), itFind));
		else
			_selectedIndex = -1;

		if (_fnDelegate and not bSilent)
			_fnDelegate(_selectedIndex);

		RefreshText();
		return _selectedIndex;
	}

	void ComboBox::RefreshText()
	{
		_bIgnoreText = true;
		if (_selectedIndex >= 0 and _selectedIndex < _items.size())
			SetText(_items[_selectedIndex]);
		else
			SetText("");
		_bIgnoreText = false;
		SetFocus(false);
	}

	void ComboBox::OnClicked()
	{
		if (_items.empty())
			return;

		auto& menu = CreateMenu();
		menu.SetStyle(MenuStyle::DropList);

		for (size_t i = 0; i < _items.size(); ++i)
		{
			auto& menuItem = menu.AddItem(_items[i]);
			menuItem.SetDelegate([this, i]() { Select(toI(i)); });
		}

		_menuId = menu.Show(GetRect());
	}
}