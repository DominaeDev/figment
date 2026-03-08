#pragma once

#include "Control.h"

namespace fig::gui
{
	class Panel : public Control
	{
	public:
		Panel(LayoutElement* pParent);

	protected:
		void OnUpdate(float fElapsed) override {};
		void OnRender(Renderer* pRenderer) override;
	};
}