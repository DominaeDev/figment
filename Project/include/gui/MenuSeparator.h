#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class MenuSeparator : public Control
	{
	public:
		MenuSeparator(LayoutElement* pParent);

	protected:
		void OnRender(Renderer* pRenderer) override;
	};
}
