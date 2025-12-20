#pragma once

#include "Control.h"

namespace fig::gui
{
	class Window;

	class Frame : public Control
	{
	public:
		Frame(Window* pHostWindow);
		virtual ~Frame();

		void Render(Renderer* pRenderer) override;

	protected:
		bool OnEvent(Event& event) override;
		virtual bool OnKeyboardEvent(SDL_KeyboardEvent& event) { return false; };
	};
}