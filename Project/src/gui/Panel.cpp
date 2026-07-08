#include <pch.h>
#include "gui/Panel.h"

using namespace fig::gui;

Panel::Panel(ParentPtr pParent) : Control(pParent)
{
}

void Panel::OnRender(Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}