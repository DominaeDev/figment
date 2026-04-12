#include <pch.h>
#include "gui/Overlay.h"
#include "gui/Frame.h"

namespace fig::gui
{
	Overlay::Overlay(Frame* pHostFrame) : Control(nullptr),
		_pOwner { pHostFrame }
	{
		SetParent(pHostFrame);
	}
} 