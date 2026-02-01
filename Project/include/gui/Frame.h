#pragma once

#include "Control.h"

namespace fig::gui
{
	class Window;

	class Frame : public Control
	{
	public:
		Frame(Window* pHostWindow);

		void Render(Renderer* pRenderer) override;
	};
}