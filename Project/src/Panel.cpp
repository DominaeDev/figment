#include "Panel.h"

Panel::Panel(Control* pParent) : Control(pParent)
{
}

void Panel::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
}