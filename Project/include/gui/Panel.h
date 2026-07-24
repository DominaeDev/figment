#pragma once

#include "Control.h"

namespace fig::gui
{
	class Panel : public Control
	{
	public:
		Panel(control_ptr pParent);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(fig::renderer_ptr pRenderer) override;
	};
}