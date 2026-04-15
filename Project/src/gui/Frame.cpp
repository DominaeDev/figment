#include <pch.h>
#include "gui/Frame.h"
#include "gui/Menu.h"
#include "gui/GUITypes.h"
#include "gui/Window.h"
#include "gui/Events.h"

namespace fig::gui
{
	Frame::Frame(Window* pHostWindow) : Control(nullptr, pHostWindow)
	{
		int w, h;
		SDL_GetWindowSizeInPixels(pHostWindow->GetSDLWindow().get(), &w, &h);
		SetSize(w, h);
	}

	Frame::~Frame()
	{
		PopAllMenus();
	}

	void Frame::Update(float fElapsed)
	{
		Control::Update(fElapsed);

		// Draw overlays
		for (int32_t i = toI(_menus.size()) - 1; i >= 0; --i)
		{
			auto& menu = _menus.at(toUZ(i));
			menu.ptr->Update(fElapsed);
		}
	}

	void Frame::Render(Renderer* pRenderer)
	{
		SDL_SetRenderDrawColor(pRenderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(pRenderer);

		Control::Render(pRenderer);

		// Draw menu
		for (auto it = _menus.cbegin(); it != _menus.cend(); ++it)
			(*it).ptr->Render(pRenderer);

		SDL_RenderPresent(pRenderer);
	}

	int32_t Frame::PushMenu(MenuPtr pMenu)
	{
		int32_t menuId = ++_nextMenuId;
		
		_menus.push_back(MenuInstance {
			.id = menuId,
			.ptr = pMenu,
		});

		// Clamp to edge
		auto& frameRect = GetRect();
		auto& menuRect = pMenu->GetRect();
		Point pos { menuRect.x, menuRect.y };

		if (menuRect.x + menuRect.w > frameRect.w)
		{
			if (_menus.size() == 1)
				pos.x = frameRect.w - menuRect.w;
			else
				pos.x = _menus.at(_menus.size() - 2).ptr->GetRect().x - menuRect.w;
		}
		if (menuRect.y < 0)
			pos.y = 0;
		if (menuRect.y + menuRect.h > frameRect.h)
			pos.y = frameRect.h - menuRect.h;
		if (menuRect.y < 0)
			pos.y = 0;

		if (pos.x != menuRect.x or pos.y != menuRect.y)
			pMenu->SetAbsolutePosition(pos);

		OnMenuOpen(menuId);
		return menuId;
	}

	void Frame::PopMenu(MenuPtr pMenu)
	{
		std::vector<int32_t> removedIds;

		auto itFind = std::find_if(_menus.begin(), _menus.end(), [pMenu](auto&& m) { return m.ptr == pMenu; });
		while (itFind != _menus.end())
		{
			removedIds.push_back((*itFind).id);
			delete (*itFind).ptr;
			itFind = _menus.erase(itFind);
		}

		for (auto it = removedIds.crbegin(); it != removedIds.crend(); it++)
			OnMenuClose(*it);
	}

	void Frame::PopAllMenus()
	{
		std::vector<int32_t> removedIds;
		for (auto menu : _menus)
		{
			removedIds.push_back(menu.id);
			delete menu.ptr;
		}
		_menus.clear();

		for (auto it = removedIds.crbegin(); it != removedIds.crend(); it++)
			OnMenuClose(*it);
	}

	bool Frame::ProcessEvent(Event& event)
	{
		for (int32_t i = toI(_menus.size()) - 1; i >= 0; --i)
		{
			if (_menus[toUZ(i)].ptr->ProcessEvent(event))
				return true;
		}

		if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN 
			&& HandleMouseDown(event.button))
			return true;
		
		return Control::ProcessEvent(event);
	}

	bool Frame::HandleMouseDown(SDL_MouseButtonEvent& event)
	{
		if (event.button == SDL_BUTTON_LEFT && !_menus.empty())
		{
			bool inside = false;
			int32_t mx = toI(event.x);
			int32_t my = toI(event.y);
			for (auto& menu : _menus)
			{
				auto rect = menu.ptr->GetRect();
				if (is_inside(rect, mx, my))
				{
					inside = true; 
					break;
				}
			}
			if (!inside)
				PopAllMenus();
		}

		return false;
	}

	void Frame::OnMenuOpen(int32_t menuId)
	{
		PushEvent(EventType::MenuOpened, menuId);
	}

	void Frame::OnMenuClose(int32_t menuId)
	{
		PushEvent(EventType::MenuClosed, menuId);
	}
}