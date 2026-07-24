#include <pch.h>
#include "gui/Panel.h"

using namespace fig::gui;

Panel::Panel(control_ptr pParent) : Control(pParent)
{
}

void Panel::OnRender(fig::renderer_ptr pRenderer)
{
	DrawBackground(pRenderer);
}