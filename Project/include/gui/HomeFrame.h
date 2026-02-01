#ifndef HOME_FRAME_H__
#define HOME_FRAME_H__
#pragma once

#include "Screen.h"
#include "GUICommon.h"

namespace fig::gui
{
	class HomeFrame : public Screen
	{
	public:
		HomeFrame(Frame* pParent);

		void CreateCards();

	protected:
		virtual void OnUpdate(float fDeltaTime) override;
		virtual void OnRender(Renderer* pRenderer) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;

	private:
		Area* _pMainArea {};
	};
}


#endif
