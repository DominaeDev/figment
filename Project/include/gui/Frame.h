#pragma once

#include "Control.h"

namespace fig::gui
{
	class Window;
	class Overlay;

	class Frame : public Control
	{
	public:
		Frame(Window* pHostWindow);
		~Frame();

		void Render(Renderer* pRenderer) override;
		void Update(float fElapsed) override;
		bool ProcessEvent(Event& event) override;

		void AddOverlay(Overlay* pOverlay);
		void DestroyOverlay(Overlay* pOverlay);
		void DestroyOverlays();

	protected:
		std::vector<Overlay*> _overlays;
	};
}