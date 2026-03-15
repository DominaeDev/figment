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
		void SetTopMargin(Coord margin) { _topMargin = margin; };
		void SetBottomMargin(Coord margin) { _bottomMargin = margin; };

	protected:
		Coord _topMargin {};
		Coord _bottomMargin {};
		float _fScrollY {};
		Coord _maxExtent {};
	};
}

#endif