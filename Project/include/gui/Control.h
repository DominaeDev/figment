#pragma once

#include "gui/Events.h"
#include "gui/GUITypes.h"
#include "LayoutElement.h"

namespace fig::gui
{
	class CustomRenderer;
	class Window;

	class Control : public LayoutElement
	{
	public:
		Control(LayoutElement* pParent);
		Control(LayoutElement* pParent, Window* pHostWindow);
		virtual ~Control();

		virtual void Render(Renderer* pRenderer);

		Color GetForegroundColor() const;
		Color GetBackgroundColor() const;
		inline bool GetClipping() const { return _bClipping; }
		virtual void SetForegroundColor(Color color) { _foregroundColor = color; }
		virtual void SetBackgroundColor(Color color) { _backgroundColor = color; }
		inline void SetBorderColor(Color color) noexcept { _borderColor = color; }
		inline void EnableClipping(bool bEnable) noexcept { _bClipping = bEnable; }
		inline void EnableCulling(bool bEnable) noexcept { _bCulling = bEnable; }

		bool GetVisible() const noexcept { return _bVisible; }
		void SetVisible(bool bVisible);
		bool GetEnabled() const { return _bEnabled; }
		void SetEnabled(bool bEnabled);
		virtual EventResult ProcessEvent(Event& event);

		void SetBackgroundRenderer(CustomRenderer* pCustom);
		void SetBorderRenderer(CustomRenderer* pCustom);

		void SetMargins(Coord left, Coord top, Coord right, Coord bottom);
		void SetMargins(Rect rect);
		Rect GetClientRect() const noexcept;
		inline void SetMarginLeft(Coord margin) noexcept { _marginLeft = margin; }
		inline void SetMarginTop(Coord margin) noexcept { _marginTop = margin; }
		inline void SetMarginRight(Coord margin) noexcept { _marginRight = margin; }
		inline void SetMarginBottom(Coord margin) noexcept { _marginBottom = margin; }
		inline Coord GetMarginLeft() const noexcept { return _marginLeft; }
		inline Coord GetMarginTop() const noexcept { return _marginTop; }
		inline Coord GetMarginRight() const noexcept { return _marginRight; }
		inline Coord GetMarginBottom() const noexcept { return _marginBottom; }

	protected:
		virtual void OnRender(Renderer* pRenderer);
		virtual void OnPostRender() {};
		virtual void OnVisibility(bool bVisible) {};
		virtual void OnEnabled(bool bEnabled) {};
		virtual void OnParent();
		virtual EventResult OnEvent(Event& event) { return EventResult::Pass; }

		void DrawBackground(Renderer* pRenderer);
		void DrawBorder(Renderer* pRenderer);
		inline Coord GetMarginHorizontal() const noexcept { return _marginLeft + _marginRight; }
		inline Coord GetMarginVertical() const noexcept { return _marginTop + _marginBottom; }

		WindowPtr GetSDLWindow();
		RendererPtr GetSDLRenderer();
		TextEnginePtr GetSDLTextEngine();
		Point GetMousePos() const noexcept;

	private:
		Color _foregroundColor {};
		Color _backgroundColor {};
		Color _borderColor {};
		bool _bClipping = false;
		bool _bCulling = false;
		bool _bVisible = true;
		bool _bEnabled = true;

		// Theming
		fig::observer_ptr<CustomRenderer> _pBGRenderer;
		fig::observer_ptr<CustomRenderer> _pBorderRenderer;
	
		// Margin
		Coord _marginLeft = 0;
		Coord _marginTop = 0;
		Coord _marginRight = 0;
		Coord _marginBottom = 0;

	private:
		struct ControlRenderContext
		{
			WindowPtr pWindow {};	// weak
			RendererPtr pRenderer {};	// weak
			TextEnginePtr pTextEngine {};	// weak
		};
		std::shared_ptr<ControlRenderContext> _renderContext {};
		std::shared_ptr<ControlRenderContext> GetRenderContext();
	};
}