#pragma once

#include "gui/Events.h"
#include "gui/GUITypes.h"
#include "gui/CustomRenderer.h"
#include "LayoutElement.h"

namespace fig::gui
{
	class Window;

	class Control : public LayoutElement
	{
	public:
		Control(ParentPtr pParent);
		Control(ParentPtr pParent, Window* pHostWindow);
		virtual ~Control() = default;

		virtual void Render(fig::renderer_ptr pRenderer);

		fig::color GetForegroundColor() const;
		fig::color GetBackgroundColor() const;
		inline bool GetClipping() const { return _bClipping; }
		virtual void SetForegroundColor(fig::color color) { _foregroundColor = color; }
		virtual void SetBackgroundColor(fig::color color) { _backgroundColor = color; }
		inline void SetBorderColor(fig::color color) noexcept { _borderColor = color; }
		inline void EnableClipping(bool bEnable) noexcept { _bClipping = bEnable; }
		inline void EnableCulling(bool bEnable) noexcept { _bCulling = bEnable; }

		bool GetVisible() const noexcept { return _bVisible; }
		void SetVisible(bool bVisible);
		bool GetEnabled() const { return _bEnabled; }
		void SetEnabled(bool bEnabled);
		virtual EventResult ProcessEvent(fig::event& event);

		void SetBackgroundRenderer(CustomRenderer* pCustom);
		void SetBorderRenderer(CustomRenderer* pCustom);
		fig::observer_ptr<CustomRenderer> GetBackgroundRenderer() { return _pBGRenderer.get(); }
		fig::observer_ptr<CustomRenderer> GetBorderRenderer() { return _pBorderRenderer.get(); }

		template <typename T, typename... Args>
			requires std::derived_from<T, CustomRenderer>
		fig::observer_ptr<T> SetBackgroundRenderer(Args&&... args)
		{
			_pBGRenderer = std::make_unique<T>(std::forward<Args>(args)...);
			return fig::observer_ptr<T>((T*)_pBGRenderer.get());
		}

		template <typename T, typename... Args>
			requires std::derived_from<T, CustomRenderer>
		fig::observer_ptr<T> SetBorderRenderer(Args&&... args)
		{
			_pBorderRenderer = std::make_unique<T>(std::forward<Args>(args)...);
			return fig::observer_ptr<T>((T*)_pBorderRenderer.get());
		}

		void ClearBackgroundRenderer();
		void ClearBorderRenderer();

		void SetMargins(fig::coord left, fig::coord top, fig::coord right, fig::coord bottom);
		void SetMargins(fig::rect rect);
		fig::rect GetClientRect() const noexcept;
		inline void SetMarginLeft(fig::coord margin) noexcept { _marginLeft = margin; }
		inline void SetMarginTop(fig::coord margin) noexcept { _marginTop = margin; }
		inline void SetMarginRight(fig::coord margin) noexcept { _marginRight = margin; }
		inline void SetMarginBottom(fig::coord margin) noexcept { _marginBottom = margin; }
		inline fig::coord GetMarginLeft() const noexcept { return _marginLeft; }
		inline fig::coord GetMarginTop() const noexcept { return _marginTop; }
		inline fig::coord GetMarginRight() const noexcept { return _marginRight; }
		inline fig::coord GetMarginBottom() const noexcept { return _marginBottom; }

	protected:
		virtual void OnRender(fig::renderer_ptr pRenderer);
		virtual void OnPostRender() {};
		virtual void OnVisibility(bool bVisible) {};
		virtual void OnEnabled(bool bEnabled) {};
		virtual void OnParent();
		virtual EventResult OnEvent(fig::event& event) { return EventResult::Pass; }

		void DrawBackground(fig::renderer_ptr pRenderer);
		void DrawBorder(fig::renderer_ptr pRenderer);
		inline fig::coord GetMarginHorizontal() const noexcept { return _marginLeft + _marginRight; }
		inline fig::coord GetMarginVertical() const noexcept { return _marginTop + _marginBottom; }

		fig::window_ptr GetSDLWindow();
		fig::renderer_ptr GetSDLRenderer();
		fig::text_engine_ptr GetSDLTextEngine();
		fig::point GetMousePos() const noexcept;

	protected:
		fig::color _foregroundColor {};
		fig::color _backgroundColor {};
		fig::color _borderColor {};
		bool _bClipping = false;
		bool _bCulling = false;
		bool _bVisible = true;
		bool _bEnabled = true;

		// Theming
		std::unique_ptr<CustomRenderer> _pBGRenderer;
		std::unique_ptr<CustomRenderer> _pBorderRenderer;
	
		// Margin
		fig::coord _marginLeft = 0;
		fig::coord _marginTop = 0;
		fig::coord _marginRight = 0;
		fig::coord _marginBottom = 0;

	private:
		struct ControlRenderContext
		{
			fig::window_ptr pWindow {};	// weak
			fig::renderer_ptr pRenderer {};	// weak
			fig::text_engine_ptr pTextEngine {};	// weak
		};
		std::shared_ptr<ControlRenderContext> _renderContext {};
		std::shared_ptr<ControlRenderContext> GetRenderContext();
	};
}