#include <pch.h>
#include "gui/SidePanel.h"

namespace fig::gui
{
	SidePanel::SidePanel(Control* parent) : Control(parent)
	{
		SetWidth(240);
		SetBackgroundColor(Color { 0xEE, 0xEC, 0xE4, 0xFF });
	}
}