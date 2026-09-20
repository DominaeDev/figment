#include <pch.h>
#include "gui/Panel.h"

namespace fig::gui
{
	Panel::Panel(ControlPtr pParent) : Control(pParent)
	{
	}

	void Panel::OnRender(fig::renderer_ptr pRenderer)
	{
		DrawBackground(pRenderer);
	}
}