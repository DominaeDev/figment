#ifndef MENU_H__
#define MENU_H__
#pragma once

#include "gui/Overlay.h"
#include "gui/BaseButton.h"

namespace fig::gui
{
	class Frame;
	using MenuDelegate = std::function<void()>;

	class MenuItem
	{
		friend class Menu;
	public:
		explicit MenuItem(const fig::string& label, TextureType icon = {}, MenuDelegate fn = {});
		
		MenuItem& AddItem(const fig::string& label, TextureType icon = {}, MenuDelegate fn = {});
		MenuItem& AddCheckItem(const fig::string& label, bool bChecked = false, MenuDelegate fn = {});

		inline MenuItem& SetLabel(const fig::string& label) noexcept { _label = label; return *this; };
		inline MenuItem& SetEnabled(bool bEnabled) noexcept { _state = bEnabled ? State::Default : State::Disabled; return *this; };
		inline MenuItem& SetIcon(TextureType icon) noexcept { _icon = icon; return *this; };
		inline MenuItem& SetDelegate(MenuDelegate delegate) noexcept { _fnDelegate = delegate; return *this; };
		inline MenuItem& SetCheckable(bool bCheckable) noexcept { _bCheckable = bCheckable; return *this; };
		inline MenuItem& SetChecked(bool bChecked) noexcept { _bChecked = _bCheckable && bChecked; return *this; };
		inline bool IsEnabled() const noexcept { return _state != State::Disabled; }
		inline bool HasSubMenu() const noexcept { return !_subItems.empty(); }

	private:
		enum class State
		{
			Default,
			Pressed,
			Hover,
			Disabled,
		} _state {};

		fig::string _label;
		TextureType _icon {};
		MenuDelegate _fnDelegate {};
		bool _bCheckable {};
		bool _bChecked {};

		std::vector<MenuItem> _subItems;
		Rect rect {};
		Control* pControl {};
	};

	class Menu : public Overlay
	{
	public:
		Menu(Frame* pHostFrame);

		MenuItem& AddItem(const fig::string& label, TextureType icon = {}, MenuDelegate fn = {});
		MenuItem& AddCheckItem(const fig::string& label, bool bChecked = false, MenuDelegate fn = {});
		void AddSeparator();

		void Show(Point position = {-1, -1});
		void Reset();

	protected:
		void OnRender(Renderer* pRenderer) override;
		bool OnEvent(Event& event) override;

		bool HandleMouseMotion(SDL_MouseMotionEvent& event);
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		bool HandleMouseUp(SDL_MouseButtonEvent& event);
		
		void CreateItem(MenuItem& menuItem);
		void CreateSeparator(MenuItem& menuItem);
		void SetMenuItemState(int32_t index, MenuItem::State state);

	protected:
		fig::sdl::Texture _texture;
		std::vector<MenuItem> _items;

		bool _bInitialized = false;

		int32_t _mouseHoverIndex = -1;
		bool _bMouseDown = false;
		int32_t _itemY = 0;
		Point _position {};

	};
}

#endif