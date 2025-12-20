#pragma once

#include "Control.h"

namespace fig::gui
{
	class Panel : public Control
	{
	public:
		Panel(Control* pParent);

	protected:
		void OnUpdate(float fDeltaTime) override {};
		void OnRender(Renderer* pRenderer) override;
	};
}