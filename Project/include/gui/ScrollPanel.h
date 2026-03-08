#ifndef SCROLL_PANEL_H__
#define SCROLL_PANEL_H__
#pragma once

#include "Control.h"

namespace fig::gui
{
	class ScrollPanel : public Control
	{
	public:
		ScrollPanel(LayoutElement* pParent);

	protected:
		bool OnEvent(Event& event) override;
		void OnAfterLayout() override;
		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void SetTopMargin(float margin) { _fTopMargin = margin; };
		void SetBottomMargin(float margin) { _fBottomMargin = margin; };

	protected:
		float _fTopMargin {};
		float _fBottomMargin {};
		float _fScrollY {};
		float _fMaxExtent {};
	};
}

#endif