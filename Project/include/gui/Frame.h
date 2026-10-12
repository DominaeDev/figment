#pragma once

#include "gui/Control.h"
#include <future>

namespace fig::gui
{
	using WindowPtr = fig::observer_ptr<class Window>;
	using OverlayPtr = fig::observer_ptr<class Overlay>;
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

		inline bool IsMenuShowing() const noexcept { return !_overlays.empty(); };
		int32_t PushOverlay(OverlayPtr pOverlay);
		void PopOverlay(OverlayPtr pOverlay);
		void PopAllOverlays();

		inline bool IsModalShowing() const noexcept { return !_modals.empty(); };
		int32_t PushModal(ModalPtr pModal);
		void PopModal(ModalPtr pModal);
		void PopAllModals();

		std::future<fig::sdl::Surface> GetSnapshot();

	protected:
		bool HandleMouseDown(SDL_MouseButtonEvent& event);
		void OnOverlayOpen(int32_t overlayId);
		void OnOverlayClose(int32_t overlayId);
		void OnSize() override;
		
		void RefreshCursor();
		void ResetCursor();
		void Flush();

	protected:
		int32_t _nextOverlayId {};
		struct OverlayInstance
		{
			int32_t id;
			OverlayPtr ptr;
		};
		std::vector<OverlayInstance> _overlays;

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