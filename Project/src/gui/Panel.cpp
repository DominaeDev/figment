#include <pch.h>
#include "gui/Panel.h"

using namespace fig::gui;

Panel::Panel(LayoutElement* pParent) : Control(pParent)
{
}

void Panel::OnRender(Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}