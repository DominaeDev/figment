#include "gui/Panel.h"

Panel::Panel(Control* pParent) : Control(pParent)
{
}

void Panel::OnRender(Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}