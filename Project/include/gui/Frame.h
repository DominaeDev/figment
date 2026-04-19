#pragma once

#include "Control.h"

namespace fig::gui
{
	class Window;
	class Menu;

	using MenuPtr = Menu*;

	class Frame : public Control
	{
	public:
		Frame(Window* pHostWindow);
		~Frame();

		void Render(Renderer* pRenderer) override;
		void Update(float fElapsed) override;
		bool ProcessEvent(Event& event) override;
		Menu& CreateMenu() noexcept;

		inline bool IsMenuShowing() const noexcept { return !_menus.empty(); };
		int32_t PushMenu(MenuPtr pMenu);
		void PopMenu(MenuPtr pMenu);
		void PopAllMenus();

	protected:
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		void OnMenuOpen(int32_t menuId);
		void OnMenuClose(int32_t menuId);

	protected:
		struct MenuInstance
		{
			int32_t id;
			MenuPtr ptr;
		};
		std::vector<MenuInstance> _menus;
		int32_t _nextMenuId {};
	};
}