#include <pch.h>
#include "gui/Overlay.h"
#include "gui/Frame.h"

namespace fig::gui
{
	Overlay::Overlay(FramePtr pHostFrame) : Control(nullptr),
		_pOwner { pHostFrame }
	{
		SetParent(pHostFrame);
	}

	void Overlay::Destroy()
	{
		_bDestroyMe = true;
	}
} 