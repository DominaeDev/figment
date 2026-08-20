#include <pch.h>
#include "gui/DropList.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorderRenderer.h"
#include "gui/Menu.h"

namespace fig::gui
{
	DropListBase::DropListBase(ControlPtr pParent) : Control(pParent), MouseEventHandler(this)
	{
		SetForegroundColor(Color::TextBoxForeground);
		
		_pText = CreateControl<StaticText>("", FontFace::Default, Constants::GUI::DefaultFontSize, false);
		_pText->EnableEllipsis(true);

		_pArrow = CreateControl<Image>(Resource::ICON_DROPLIST_ARROW);
		_pArrow->SetForegroundColor(Color::Icon);

		auto pTextBoxBG = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_6PX, 8);
		pTextBoxBG->SetExtend(0.0f);
		pTextBoxBG->SetColor(Color::TextBoxBackground);

		auto pTextBoxBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_6PX, 8);
		pTextBoxBorder->SetExtend(0.0f);
		pTextBoxBorder->SetColor(Color::LineColor);

		SetSize(300, 32);
	}

	EventResult DropListBase::OnEvent(fig::event& event)
	{
		if (auto result = MouseEventHandler::HandleMouseEvents(event); result == EventResult::Handled)
			return result;

		return Control::OnEvent(event);
	}

	void DropListBase::OnSize()
	{
		_pText->SetX(12);
		_pText->SetWidth(GetWidth() - 24 - _pArrow->GetWidth());
		_pText->CenterVertically();

		_pArrow->SetX(GetWidth() - 8 - _pArrow->GetWidth());
		_pArrow->CenterVertically();
	}

	void DropListBase::RefreshText()
	{
		if (_selectedIndex >= 0 and _selectedIndex < _items.size())
			_pText->SetText(_items[_selectedIndex]);
		else
			_pText->SetText("");
	}

	int32_t DropListBase::Select(int32_t index, bool bSilent) noexcept
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

	int32_t DropListBase::SelectValue(fig::string_view label, bool bSilent) noexcept
	{
		if (auto itFind = std::find_if(_items.cbegin(), _items.cend(), [label](auto&& i) { return i == label; }); itFind != _items.cend())
			_selectedIndex = static_cast<int32_t>(std::distance(_items.cbegin(), itFind));
		else
			_selectedIndex = -1;
		
		if (_fnDelegate and not bSilent)
			_fnDelegate(_selectedIndex);

		RefreshText();
		return _selectedIndex;
	}

	int32_t DropListBase::GetSelectedIndex() const noexcept
	{
		return _selectedIndex;
	}

	void DropListBase::SetDelegate(ItemSelectedDelegate fnDelegate) 
	{ 
		_fnDelegate = fnDelegate; 
	}

	void DropListBase::OnClicked()
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