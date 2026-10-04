#pragma once

#include "gui/Control.h"
#include <future>

namespace fig::gui
{
	using WindowPtr = fig::observer_ptr<class Window>;
	using OverlayPtr = fig::observer_ptr<class Overlay>;
	using MenuPtr = fig::observer_ptr<class Menu>;
	using ModalPtr = fig::observer_ptr<class ModalOverlay>;

	class Frame : public Control
	{
	public:
		Frame(WindowPtr pHostWindow);
		~Frame();

		void Render(fig::renderer_ptr pRenderer) override;
		void Update(float fElapsed);
		EventResult ProcessEvent(fig::event& event) override;
		Menu& CreateMenu() noexcept;
		
		template <typename T, typename... Args>
		requires std::derived_from<T, ModalOverlay>
		T& CreateModal(Args&&... args) noexcept
		{
			return *new T(this, std::forward<Args>(args)...);
		}

		inline bool IsMenuShowing() const noexcept { return !_menus.empty(); };
		int32_t PushMenu(MenuPtr pMenu);
		void PopMenu(MenuPtr pMenu);
		void PopAllMenus();

		inline bool IsModalShowing() const noexcept { return !_modals.empty(); };
		int32_t PushModal(ModalPtr pModal);
		void PopModal(ModalPtr pModal);
		void PopAllModals();

		std::future<fig::sdl::Surface> GetSnapshot();

	protected:
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		void OnMenuOpen(int32_t overlayId);
		void OnMenuClose(int32_t overlayId);
		void OnSize() override;
		
		void RefreshCursor();
		void ResetCursor();
		void Flush();

	protected:
		int32_t _nextOverlayId {};
		struct MenuInstance
		{
			int32_t id;
			MenuPtr ptr;
		};
		std::vector<MenuInstance> _menus;

		struct ModalInstance
		{
			int32_t id;
			ModalPtr ptr;
		};
		std::vector<ModalInstance> _modals;
		
		std::vector<OverlayPtr> _removalQueue;
		
		std::vector<fig::cursor> _cursors;
		std::vector<std::promise<fig::sdl::Surface>> _snapshotPromises;
	};
}