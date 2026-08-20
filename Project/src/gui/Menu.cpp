#include <pch.h>
#include "gui/Menu.h"
#include "gui/Frame.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"
#include "gui/CustomRenderers.h"
#include "gui/MenuSeparator.h"
#include "app/AppState.h"
#include "gui/Events.h"

namespace fig::gui
{
	constexpr int32_t MenuMargin = 4;
	constexpr int32_t MenuWidth = 240;
	constexpr int32_t MenuItemHeight = 30;
	constexpr int32_t MenuSeparatorHeight = 5;
	constexpr int32_t MenuSeparatorMargin = 8;
	constexpr fig::color MenuBackgroundColor = Color::White;
	constexpr fig::color MenuBorderColor = Color::LineColor;
	constexpr fig::color MenuItemHoverColor = fig::color { 0xefece3, 0x80 };
	constexpr fig::color MenuItemPressedColor = fig::color { 0xefece3, 0xFF };
	constexpr float AutoExpandDelay = 0.3f;
	constexpr float AutoCollapseDelay = 0.3f;
	constexpr const char* Separator = "----";

	MenuItem::MenuItem(const fig::string& label, Resource icon, MenuDelegate fn) :
		_label { label },
		_icon { icon },
		_fnDelegate { fn }
	{
	}

	MenuItem& MenuItem::AddItem(const fig::string& label, Resource icon, MenuDelegate fn)
	{
		_subItems.emplace_back(MenuItem(label, icon, fn));
		return _subItems.back();
	}

	MenuItem& MenuItem::AddCheckItem(const fig::string& label, bool bChecked, MenuDelegate fn)
	{
		auto menuItem = MenuItem(label, Resource::NONE, fn);
		menuItem._bCheckable = true;
		menuItem._bChecked = bChecked;
		_subItems.emplace_back(menuItem);
		return _subItems.back();
	}

	void MenuItem::AddSeparator()
	{
		_subItems.emplace_back(MenuItem(Separator));
		_subItems.back().SetEnabled(false);
	}

	Menu::Menu(Frame* pHostFrame) : Overlay(pHostFrame)
	{
		auto pBackground = SetBackgroundRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BACKGROUND_10PX, 16);
		pBackground->SetColor(MenuBackgroundColor);

		auto pBorder = SetBorderRenderer<TexturedBorderRenderer>(Resource::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Color::LineColor);

		SetBackgroundColor(Color::White);
		SetForegroundColor(Color::Black);

		SetSize(MenuWidth, MenuItemHeight);
		SetVisible(false);
	}

	MenuItem& Menu::AddItem(const fig::string& label, Resource icon, MenuDelegate fn)
	{
		_items.emplace_back(MenuItem(label, icon, fn));
		return _items.back();
	}

	MenuItem& Menu::AddCheckItem(const fig::string& label, bool bChecked, MenuDelegate fn)
	{
		auto menuItem = MenuItem(label, Resource::NONE, fn);
		menuItem._bCheckable = true;
		menuItem._bChecked = bChecked;
		_items.emplace_back(menuItem);
		return _items.back();
	}

	void Menu::AddSeparator()
	{
		_items.emplace_back(MenuItem(Separator));
		_items.back().SetEnabled(false);
	}

	void Menu::OnUpdate(float fElapsed)
	{
		_fExpandTimer += fElapsed;
		if (_fExpandTimer >= AutoExpandDelay)
		{
			_fExpandTimer = 0.0f; // jic

			// Show hovered submenu
			if (_mouseHoverIndex != -1 && _submenuIndex != _mouseHoverIndex)
			{
				if (_items[_mouseHoverIndex].HasSubMenu())
					ShowSubmenu(_mouseHoverIndex);
			}
		}

		if (_pSubmenu)
		{
			if (_mouseHoverIndex > 0)
			{
				if (_mouseHoverIndex != _submenuIndex)
					_fCollapseTimer += fElapsed;
				else
					_fCollapseTimer = 0.0f;
			}

			if (_fCollapseTimer >= AutoCollapseDelay)
			{
				_pOwner->PopMenu(_pSubmenu);
				_pSubmenu = nullptr;
				_submenuIndex = -1;
			}
		}
	}

	void Menu::OnRender(fig::renderer_ptr pRenderer)
	{
		Control::OnRender(pRenderer);
		return;
	}

	void Menu::CreateItems()
	{
		Reset();

		for (auto& item : _items)
		{
			if (item._label == Separator)
				CreateSeparator(item);
			else
				CreateItem(item);
		}

		_bInitialized = true;
	}

	uint32_t Menu::Show(fig::rect parentRect, bool bPopAll)
	{
		if (_pOwner and bPopAll)
			_pOwner->PopAllMenus();

		SetWidth(parentRect.w);

		CreateItems();

		auto menuY = parentRect.y + parentRect.h;
		auto menuHeight = GetHeight();
		auto maxY = _pOwner->GetHeight();
		if (menuHeight >= maxY)
			menuY = 0;
		else if (menuY + menuHeight > maxY)
			menuY = parentRect.y - menuHeight;
		menuY = std::max(menuY, 0);

		SetAbsolutePosition(fig::point { parentRect.x, menuY });
		SetVisible(true);

		int32_t menuId = _pOwner->PushMenu(this);
		return menuId;
	}

	uint32_t Menu::Show(fig::point position, bool bPopAll)
	{
		if (_pOwner and bPopAll)
			_pOwner->PopAllMenus();

		CreateItems();

		if (position.x == -1 and position.y == -1)
		{
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			position = fig::point { toI(mx), toI(my) };
		}

		SetAbsolutePosition(position);
		SetVisible(true);

		int32_t menuId = _pOwner->PushMenu(this);
		return menuId;
	}

	void Menu::CreateItem(MenuItem& menuItem)
	{
		auto pItemRoot = CreateControl<TexturedBorder>(AppResources::GetTexture(Resource::ROUNDED_BACKGROUND_6PX), 8);
		pItemRoot->SetPosition(MenuMargin, MenuMargin + _itemY);
		pItemRoot->SetSize(GetWidth() - MenuMargin * 2, MenuItemHeight);
		pItemRoot->SetForegroundColor(MenuBackgroundColor);

		if (_style == MenuStyle::Default)
		{
			auto pItemLabel = pItemRoot->CreateControl<StaticText>(menuItem._label, FontFace::Default, 14.5, false);
			pItemLabel->EnableEllipsis(true);
			pItemLabel->SetPosition(32, 5);
			pItemLabel->SetMaxWidth(pItemRoot->GetWidth() - 36);
			pItemLabel->SetWidth(pItemRoot->GetWidth() - 36);
			pItemLabel->SetForegroundColor(menuItem.IsEnabled() ? Color::SidePanelForeground : Color::DisabledForeground);
		}
		else if (_style == MenuStyle::DropList)
		{
			auto pItemLabel = pItemRoot->CreateControl<StaticText>(menuItem._label, FontFace::Default, Constants::GUI::DefaultFontSize, false);
			pItemLabel->EnableEllipsis(true);
			pItemLabel->SetPosition(6, 3);
			pItemLabel->SetMaxWidth(pItemRoot->GetWidth() - 12);
			pItemLabel->SetWidth(pItemRoot->GetWidth() - 12);
			pItemLabel->SetForegroundColor(menuItem.IsEnabled() ? Color::SidePanelForeground : Color::DisabledForeground);
		}

		if (menuItem._bCheckable && menuItem._bChecked)
		{
			auto pIcon = pItemRoot->CreateControl<Image>(AppResources::GetTexture(Resource::ICON_CHECKMARK));
			pIcon->SetForegroundColor(menuItem.IsEnabled() ? Color::Icon : Color::DisabledForeground);
			pIcon->SetPosition(4, 4);
		}
		else if (menuItem._icon != Resource::NONE)
		{
			auto pIcon = pItemRoot->CreateControl<Image>(AppResources::GetTexture(menuItem._icon));
			if (menuItem._bMonochromeIcon)
				pIcon->SetForegroundColor(menuItem.IsEnabled() ? Color::Icon : Color::DisabledForeground);
			else
				pIcon->SetForegroundColor(menuItem.IsEnabled() ? Color::White : Color::White.WithAlpha(0x80));
			pIcon->SetPosition(4, 4);
		}

		if (menuItem.HasSubMenu())
		{
			auto pArrow = pItemRoot->CreateControl<Image>(AppResources::GetTexture(Resource::SUBMENU_ARROW));
			pArrow->SetForegroundColor(menuItem.IsEnabled() ? Color::Icon : Color::DisabledForeground);
			pArrow->SetX(pItemRoot->GetWidth() - pArrow->GetWidth());
			pArrow->CenterVertically();
		}

		_itemY += MenuItemHeight;
		SetHeight(_itemY + MenuMargin * 2);

		menuItem.rect = pItemRoot->GetRect();
		menuItem.pControl = pItemRoot;
	}

	void Menu::CreateSeparator(MenuItem& menuItem)
	{
		auto pItemRoot = CreateControl<MenuSeparator>();
		pItemRoot->SetPosition(MenuSeparatorMargin, MenuMargin + _itemY);
		pItemRoot->SetSize(MenuWidth - MenuSeparatorMargin * 2, MenuSeparatorHeight);
		pItemRoot->SetForegroundColor(Color::LineColor);

		_itemY += MenuSeparatorHeight;
		SetSize(MenuWidth, _itemY + MenuMargin * 2);

		menuItem.rect = pItemRoot->GetRect();
		menuItem.pControl = pItemRoot;
	}

	void Menu::Reset()
	{
		DestroyChildren();
		_pSubmenu = nullptr;
		_submenuIndex = -1;
		_fExpandTimer = 0.0f;
		_fCollapseTimer = 0.0f;
	}

	EventResult Menu::OnEvent(fig::event& event)
	{
		if (!_bInitialized)
			return EventResult::Pass;

		switch (event.type)
		{
		case SDL_EVENT_MOUSE_MOTION:
			return HandleMouseMotion(event.motion) ? EventResult::Handled : EventResult::Pass;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			return HandleMouseDown(event.button) ? EventResult::Handled : EventResult::Pass;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			return HandleMouseUp(event.button) ? EventResult::Handled : EventResult::Pass;
		}
		return EventResult::Pass;
	}

	bool Menu::HandleMouseMotion(SDL_MouseMotionEvent& motionEvent)
	{
		auto rect = GetRect();
		if (not is_inside(rect, toI(motionEvent.x), toI(motionEvent.y)))
		{
			if (_mouseHoverIndex >= 0)
			{
				SetMenuItemState(_mouseHoverIndex, MenuItem::State::Default);
				_mouseHoverIndex = -1;
				_fExpandTimer = 0.0f;
				_fCollapseTimer = 0.0f;
			}
			return false;
		}

		int32_t hoverIndex = -1;
		for (size_t i = 0; i < _items.size(); ++i)
		{
			if (is_inside(_items[i].rect, toI(motionEvent.x - GetAbsoluteX()), toI(motionEvent.y - GetAbsoluteY())))
			{
				hoverIndex = toI(i);
				break;
			}
		}

		if (_mouseHoverIndex >= 0 && _mouseHoverIndex != hoverIndex && _items[_mouseHoverIndex].IsEnabled())
		{
			SetMenuItemState(_mouseHoverIndex, MenuItem::State::Default);
		}

		if (_mouseHoverIndex != hoverIndex)
			_fExpandTimer = 0.0f;

		_mouseHoverIndex = hoverIndex;

		if (hoverIndex >= 0 && _items[hoverIndex].IsEnabled())
		{
			if (_bMouseDown)
				SetMenuItemState(hoverIndex, MenuItem::State::Pressed);
			else
				SetMenuItemState(hoverIndex, MenuItem::State::Hover);
			return true;
		}

		return false;
	}

	bool Menu::HandleMouseDown(SDL_MouseButtonEvent& event)
	{
		auto rect = GetRect();
		if (not is_inside(rect, toI(event.x), toI(event.y)))
			return false;

		if (event.button != SDL_BUTTON_LEFT)
			return false;

		int32_t hoverIndex = -1;
		for (size_t i = 0; i < _items.size(); ++i)
		{
			if (is_inside(_items[i].rect, toI(event.x - GetAbsoluteX()), toI(event.y - GetAbsoluteY())))
			{
				hoverIndex = toI(i);
				break;
			}
		}

		_bMouseDown = true;
		if (hoverIndex >= 0 && _items[hoverIndex].IsEnabled())
			SetMenuItemState(hoverIndex, MenuItem::State::Pressed);

		return true;
	}

	bool Menu::HandleMouseUp(SDL_MouseButtonEvent& event)
	{
		bool bLeftClick = false;
		if (event.button == SDL_BUTTON_LEFT)
		{
			bLeftClick = _bMouseDown;
			_bMouseDown = false;
		}

		if (_mouseHoverIndex < 0)
			return false;

		auto& menuItem = _items[_mouseHoverIndex];
		if (not menuItem.IsEnabled())
			return false;

		SetMenuItemState(_mouseHoverIndex, MenuItem::State::Hover);

		if (menuItem.HasSubMenu())
		{
			ShowSubmenu(_mouseHoverIndex);
			return true;
		}
		
		if (menuItem._fnDelegate)
			menuItem._fnDelegate();

		_bDestroyMe = true;
		return true;
	}

	void Menu::SetMenuItemState(int32_t index, MenuItem::State state)
	{
		if (index < 0 or index >= toI(_items.size()))
			return;

		auto& menuItem = _items[index];
		menuItem._state = state;

		switch (state)
		{
		case MenuItem::State::Default:
			menuItem.pControl->SetForegroundColor(MenuBackgroundColor);
			break;
		case MenuItem::State::Hover:
			menuItem.pControl->SetForegroundColor(MenuItemHoverColor);
			break;
		case MenuItem::State::Pressed:
			menuItem.pControl->SetForegroundColor(MenuItemPressedColor);
			break;
		case MenuItem::State::Disabled:
			menuItem.pControl->SetForegroundColor(MenuBackgroundColor);
			break;
		}
	}

	void Menu::ShowSubmenu(size_t menuItemIndex)
	{
		if (_pSubmenu)
		{
			_pOwner->PopMenu(_pSubmenu);
			_pSubmenu = nullptr;
			_submenuIndex = -1;
		}

		if (menuItemIndex >= _items.size())
			return;

		auto& menuItem = _items[menuItemIndex];
		_submenuIndex = toI(menuItemIndex);
		_pSubmenu = new Menu(_pOwner);
		_pSubmenu->_items = menuItem._subItems;
		_pSubmenu->Show(fig::point { GetX() + GetWidth(), GetY() + menuItem.rect.y }, false);
	}
}