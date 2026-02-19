#ifndef HOME_FRAME_H__
#define HOME_FRAME_H__
#pragma once

#include "Screen.h"
#include "GUICommon.h"

namespace fig::gui
{
	class ScrollPanel;

	class HomeFrame : public Screen
	{
	public:
		HomeFrame(Frame* pParent);

		void CreateCards();

		TYPE_ID(1);
	protected:
		virtual void OnRender(Renderer* pRenderer) override;

		bool OnKeyboardEvent(KeyboardEvent& event) override;

	private:
		ScrollPanel* _pMainArea {};
		Area* _pScrollArea {};
		float _fScrollY {};
	};
}


#endif
