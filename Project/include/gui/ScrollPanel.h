#ifndef SCROLL_PANEL_H__
#define SCROLL_PANEL_H__
#pragma once

#include "Control.h"

namespace fig::gui
{
	class VerticalScrollBar;

	class ScrollPanel : public Control
	{
	public:
		ScrollPanel(LayoutElement* pParent, bool bScrollBar = true);
		~ScrollPanel();
		void Render(Renderer* pRenderer) override;
		inline void SetScrollBarOffset(Coord offset) noexcept { _scrollBarOffset = offset; }
		
		void ScrollTo(float position) noexcept;

	protected:
		void OnUpdate(float fElapsed) override;
		bool OnEvent(Event& event) override;
		void OnAfterLayout() override;
		virtual void OnScroll() {};

		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void SetTopMargin(Coord margin) { _topMargin = margin; };
		void SetBottomMargin(Coord margin) { _bottomMargin = margin; };
		void RefreshScrollBar();

	protected:
		Coord _topMargin {};
		Coord _bottomMargin {};
		float _fScrollY {};
		float _fTargetScrollY {};
		Coord _maxExtent {};
		Coord _scrollBarOffset { -16 };

	private:
		VerticalScrollBar* _pScrollBar {};
	};
}

#endif