#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class MenuSeparator : public Control
	{
	public:
		MenuSeparator(control_ptr pParent);

	protected:
		void OnRender(fig::renderer_ptr pRenderer) override;
	};
}
