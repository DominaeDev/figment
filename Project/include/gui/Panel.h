#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class Panel : public Control
	{
	public:
		Panel(ControlPtr pParent);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(fig::renderer_ptr pRenderer) override;
	};
}