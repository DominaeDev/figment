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
		
		void ScrollTo(float position, bool bSmooth = true) noexcept;

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(Event& event) override;
		void OnAfterLayout() override;
		
		virtual void OnScroll() {};
		void ResetScroll() noexcept;

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
		fig::observer_ptr<VerticalScrollBar> _pScrollBar;
	};
}
