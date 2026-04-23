#ifndef DEBUG_FRAME_H__
#define DEBUG_FRAME_H__
#pragma once

#include "Screen.h"
#include "GUICommon.h"

namespace fig::gui
{
	class DebugScreen : public Screen
	{
	public:
		DebugScreen(Frame* pParent);

	protected:
		void OnUpdate(float fElapsed) override;
		void OnRender(Renderer* pRenderer) override;
		bool OnKeyboardEvent(KeyboardEvent& event) override;
	};
}


#endif
