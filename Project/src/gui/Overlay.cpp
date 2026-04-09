#include <pch.h>
#include "gui/Overlay.h"
#include "gui/Frame.h"

namespace fig::gui
{
	Overlay::Overlay(Frame* pHostFrame) : Control(nullptr),
		_pHostFrame { pHostFrame }
	{
		_pHostFrame->AddOverlay(this);
		SetParent(pHostFrame);
	}

	void Overlay::Destroy()
	{
		_bDestroyMe = true;
	}
} 