#pragma once

#include "Control.h"

namespace fig::gui
{
	class Panel : public Control
	{
	public:
		Panel(ParentPtr pParent);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(fig::renderer_ptr pRenderer) override;
	};
}