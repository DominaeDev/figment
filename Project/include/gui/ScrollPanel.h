#ifndef SCROLL_PANEL_H__
#define SCROLL_PANEL_H__
#pragma once

#include "Control.h"

namespace fig::gui
{
	class ScrollPanel : public Control
	{
	public:
		ScrollPanel(Control* pParent);

	protected:
		bool OnEvent(Event& event) override;
		void OnAfterLayout() override;
		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void SetBottomMargin(float margin) { _fBottomMargin = margin; };

	protected:
		float _fBottomMargin {};
		float _fScrollY {};
		float _fMaxExtent {};
	};
}

#endif