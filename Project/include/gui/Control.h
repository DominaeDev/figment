#pragma once

#include "Types.h"
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
		virtual void Update(float fDeltaTime) override;

		Color GetForegroundColor() const;
		Color GetBackgroundColor() const;
		bool GetClipping() const { return _bClipping; }
		virtual void SetForegroundColor(Color color) { _foregroundColor = color; }
		virtual void SetBackgroundColor(Color color) { _backgroundColor = color; }
		void SetBorderColor(Color color) { _borderColor = color; }
		void EnableClipping(bool bEnable) { _bClipping = bEnable; }
		void EnableCulling(bool bEnable) { _bCulling = bEnable; }

		bool GetVisible() { return _bVisible; }
		void SetVisible(bool bVisible) { _bVisible = bVisible; }

		bool ProcessEvent(Event& event);

		void SetBackgroundRenderer(CustomRenderer* pCustom);
		void SetBorderRenderer(CustomRenderer* pCustom);

	protected:
		virtual void OnRender(Renderer* pRenderer);
		virtual void OnPostRender() {};
		virtual void OnParent();
		virtual bool OnEvent(Event& event) { return false; }

		void DrawBackground(Renderer* pRenderer);
		void DrawBorder(Renderer* pRenderer);

		inline WindowPtr GetSDLWindow() { return _renderContext.pWindow; }
		inline RendererPtr GetSDLRenderer() { return _renderContext.pRenderer; }
		inline TextEnginePtr GetSDLTextEngine() { return _renderContext.pTextEngine; }

	private:
		Color _foregroundColor {};
		Color _backgroundColor {};
		Color _borderColor {};
		bool _bClipping = false;
		bool _bCulling = false;
		bool _bVisible = true;

		// Theming
		CustomRenderer* _pBGRenderer = nullptr;
		CustomRenderer* _pBorderRenderer = nullptr;
	
	private:
		struct ControlRenderContext
		{
			WindowPtr pWindow {};	// weak
			RendererPtr pRenderer {};	// weak
			TextEnginePtr pTextEngine {};	// weak
		};
		ControlRenderContext _renderContext {};
	};
}