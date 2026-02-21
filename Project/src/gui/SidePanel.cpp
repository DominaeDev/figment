#include <pch.h>
#include "gui/SidePanel.h"

namespace fig::gui
{
	SidePanel::SidePanel(Control* parent) : Control(parent)
	{
		SetWidth(Constants::GUI::SidePanel::Width);
		SetBackgroundColor(Color { 0xEE, 0xEC, 0xE4, 0xFF });
	}
}