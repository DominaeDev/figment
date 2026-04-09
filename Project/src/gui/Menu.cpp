#include <pch.h>
#include "gui/Menu.h"
#include "gui/Frame.h"
#include "gui/AppResources.h"
#include "gui/TexturedBorder.h"
#include "gui/CustomRenderers.h"
#include "gui/MenuSeparator.h"

using namespace fig::util;

namespace fig::gui
{
	constexpr int32_t MenuMargin = 4;
	constexpr int32_t MenuWidth = 240;
	constexpr int32_t MenuItemHeight = 30;
	constexpr int32_t MenuSeparatorHeight = 5;
	constexpr int32_t MenuSeparatorMargin = 8;
	constexpr Color MenuBackgroundColor = Colors::White;
	constexpr Color MenuBorderColor = Colors::LineColor;
	constexpr Color MenuItemHoverColor = Color { 0xefece3, 0x80 };
	constexpr Color MenuItemPressedColor = Color { 0xefece3, 0xFF };

	MenuItem::MenuItem(const fig::string& label, TextureType icon, MenuDelegate fn) :
		_label { label },
		_icon { icon },
		_fnDelegate { fn }
	{
	}

	MenuItem& MenuItem::AddItem(const fig::string& label, TextureType icon, MenuDelegate fn)
	{
		_subItems.emplace_back(MenuItem(label, icon, fn));
		return _subItems.back();
	}

	Menu::Menu(Frame* pHostFrame) : Overlay(pHostFrame)
	{
		auto pBackground = new TexturedBorderRenderer(TextureType::ROUNDED_BACKGROUND_10PX, 16);
		pBackground->SetColor(MenuBackgroundColor);
		SetBackgroundRenderer(pBackground);

		auto pBorder = new TexturedBorderRenderer(TextureType::ROUNDED_BORDER_10PX, 16);
		pBorder->SetColor(Colors::LineColor);
		SetBorderRenderer(pBorder);

		SetBackgroundColor(Colors::White);
		SetForegroundColor(Colors::Black);

		SetVisible(false);
	}

	MenuItem& Menu::AddItem(const fig::string& label, TextureType icon, MenuDelegate fn)
	{
		_items.emplace_back(MenuItem(label, icon, fn));
		return _items.back();
	}

	void Menu::AddSeparator()
	{
		_items.emplace_back(MenuItem("----"));
		_items.back().SetEnabled(false);
	}

	void Menu::OnRender(Renderer* pRenderer)
	{
		Control::OnRender(pRenderer);
		return;
	}

	void Menu::Show(Point position)
	{
		_position = position;
		if (_position.x == -1 and _position.y == -1)
		{
			float mx, my;
			SDL_GetMouseState(&mx, &my);
			_position = Point { toI(mx), toI(my) };
		}

		Reset();

		for (auto& item : _items)
		{
			if (item._label == "----")
				CreateSeparator(item);
			else
				CreateItem(item);
		}

		_bInitialized = true;

		SetAbsolutePosition(_position);
		SetVisible(true);
	}

	void Menu::CreateItem(MenuItem& menuItem)
	{
		auto pItemRoot = new TexturedBorder(this, AppResources::GetTexture(TextureType::ROUNDED_BACKGROUND_6PX), 8);
		pItemRoot->SetPosition(MenuMargin, MenuMargin + _itemY);
		pItemRoot->SetSize(MenuWidth - MenuMargin * 2, MenuItemHeight);
		pItemRoot->SetForegroundColor(MenuBackgroundColor);

		auto pItemLabel = new StaticText(pItemRoot, menuItem._label, FontFace::Default, 14.5, false);
		pItemLabel->EnableEllipsis(true);
		pItemLabel->SetPosition(32, 5);
		pItemLabel->SetMaxSize(pItemRoot->GetWidth() - 36, -1);
		pItemLabel->SetForegroundColor(menuItem.IsEnabled() ? Colors::SidePanelForeground : Colors::DisabledForeground);

		if (menuItem._icon != TextureType::NONE)
		{
			auto pIcon = new Image(pItemRoot, AppResources::GetTexture(menuItem._icon));
			pIcon->SetForegroundColor(menuItem.IsEnabled() ? Colors::SidePanelForeground : Colors::DisabledForeground);
			pIcon->SetSize(22, 22);
			pIcon->SetPosition(4, 4);
		}

		_itemY += MenuItemHeight;
		SetSize(MenuWidth, _itemY + MenuMargin * 2);

		menuItem.rect = pItemRoot->GetRect();
		menuItem.pControl = pItemRoot;
	}

	void Menu::CreateSeparator(MenuItem& menuItem)
	{
		auto pItemRoot = new MenuSeparator(this);
		pItemRoot->SetPosition(MenuSeparatorMargin, MenuMargin + _itemY);
		pItemRoot->SetSize(MenuWidth - MenuSeparatorMargin * 2, MenuSeparatorHeight);
		pItemRoot->SetForegroundColor(Colors::LineColor);

		_itemY += MenuSeparatorHeight;
		SetSize(MenuWidth, _itemY + MenuMargin * 2);

		menuItem.rect = pItemRoot->GetRect();
		menuItem.pControl = pItemRoot;
	}

	void Menu::Reset()
	{
		DestroyChildren();
	}

	bool Menu::OnEvent(Event& event)
	{
		if (!_bInitialized)
			return false;

		switch (event.type)
		{
		case SDL_EVENT_MOUSE_MOTION:
			return HandleMouseMotion(event.motion);
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			return HandleMouseDown(event.button);
		case SDL_EVENT_MOUSE_BUTTON_UP:
			return HandleMouseUp(event.button);
		}
		return false;
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
			}
			return false;
		}

		int32_t hoverIndex = -1;
		for (size_t i = 0; i < _items.size(); ++i)
		{
			if (is_inside(_items[i].rect, toI(motionEvent.x - _position.x), toI(motionEvent.y - _position.y)))
			{
				hoverIndex = toI(i);
				break;
			}
		}

		if (_mouseHoverIndex >= 0 && _mouseHoverIndex != hoverIndex && _items[_mouseHoverIndex].IsEnabled())
		{
			SetMenuItemState(_mouseHoverIndex, MenuItem::State::Default);
		}

		if (hoverIndex >= 0 && _items[hoverIndex].IsEnabled())
		{
			if (_bMouseDown)
				SetMenuItemState(hoverIndex, MenuItem::State::Pressed);
			else
				SetMenuItemState(hoverIndex, MenuItem::State::Hover);
		}
		
		_mouseHoverIndex = hoverIndex;

		return true;
	}

	bool Menu::HandleMouseDown(SDL_MouseButtonEvent& event)
	{
		auto rect = GetRect();
		if (not is_inside(rect, toI(event.x), toI(event.y)))
		{
			Destroy();
			return false;
		}

		if (event.button != SDL_BUTTON_LEFT)
			return false;

		int32_t hoverIndex = -1;
		for (size_t i = 0; i < _items.size(); ++i)
		{
			if (is_inside(_items[i].rect, toI(event.x - _position.x), toI(event.y - _position.y)))
			{
				hoverIndex = toI(i);
				break;
			}
		}

		_bMouseDown = true;
		if (hoverIndex >= 0 && _items[hoverIndex].IsEnabled())
		{
			SetMenuItemState(hoverIndex, MenuItem::State::Pressed);
		}		

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

		SetMenuItemState(_mouseHoverIndex, MenuItem::State::Hover);

		if (_items[_mouseHoverIndex].HasSubMenu())
		{
			return true;
		}
		else if (_items[_mouseHoverIndex]._fnDelegate)
		{
			_items[_mouseHoverIndex]._fnDelegate();
		}
		Destroy();
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

}