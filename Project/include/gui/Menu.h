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
		explicit MenuItem(const fig::string& label, Resource icon = {}, MenuDelegate fn = {});
		
		MenuItem& AddItem(const fig::string& label, Resource icon = {}, MenuDelegate fn = {});
		MenuItem& AddCheckItem(const fig::string& label, bool bChecked = false, MenuDelegate fn = {});
		void AddSeparator();

		inline MenuItem& SetLabel(const fig::string& label) noexcept { _label = label; return *this; };
		inline MenuItem& SetEnabled(bool bEnabled) noexcept { _state = bEnabled ? State::Default : State::Disabled; return *this; };
		inline MenuItem& SetIcon(Resource icon, bool bMonochrome = true) noexcept { _icon = icon; _bMonochromeIcon = bMonochrome; return *this; };
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
		Resource _icon {};
		MenuDelegate _fnDelegate {};
		bool _bCheckable {};
		bool _bChecked {};
		bool _bMonochromeIcon { true };

		std::vector<MenuItem> _subItems;
		fig::rect rect {};
		Control* pControl {};
	};

	class Menu : public Overlay
	{
	public:
		Menu(Frame* pHostFrame);

		MenuItem& AddItem(const fig::string& label, Resource icon = {}, MenuDelegate fn = {});
		MenuItem& AddCheckItem(const fig::string& label, bool bChecked = false, MenuDelegate fn = {});
		void AddSeparator();

		uint32_t Show(fig::point position = {-1, -1}, bool bPopAll = true);
		void Reset();

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(fig::renderer_ptr pRenderer) override;
		EventResult OnEvent(fig::event& event) override;

		bool HandleMouseMotion(SDL_MouseMotionEvent& event);
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		bool HandleMouseUp(SDL_MouseButtonEvent& event);
		
		void CreateItem(MenuItem& menuItem);
		void CreateSeparator(MenuItem& menuItem);
		void SetMenuItemState(int32_t index, MenuItem::State state);
		void ShowSubmenu(size_t menuItemIndex);

	protected:
		fig::sdl::Texture _texture;
		std::vector<MenuItem> _items;
		bool _bInitialized = false;

		int32_t _mouseHoverIndex = -1;
		bool _bMouseDown = false;
		int32_t _itemY = 0;
		float _fExpandTimer {};
		float _fCollapseTimer {};
		int32_t _submenuIndex = -1;

		fig::observer_ptr<Menu> _pSubmenu;
	};
}
