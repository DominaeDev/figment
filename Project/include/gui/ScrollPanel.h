#pragma once

#include "Control.h"

namespace fig::gui
{
	class VerticalScrollBar;

	class ScrollPanel : public Control
	{
	public:
		ScrollPanel(control_ptr pParent, bool bScrollBar = true);
		~ScrollPanel();
		void Render(fig::renderer_ptr pRenderer) override;
		inline void SetScrollBarOffset(fig::coord offset) noexcept { _scrollBarOffset = offset; }
		
		void ScrollTo(float position, bool bSmooth = true) noexcept;

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;
		void OnAfterLayout() override;
		
		virtual void OnScroll() {};
		void ResetScroll() noexcept;

		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void SetTopMargin(fig::coord margin) { _topMargin = margin; };
		void SetBottomMargin(fig::coord margin) { _bottomMargin = margin; };
		void RefreshScrollBar();

	protected:
		fig::coord _topMargin {};
		fig::coord _bottomMargin {};
		float _fScrollY {};
		float _fTargetScrollY {};
		fig::coord _maxExtent {};
		fig::coord _scrollBarOffset { -16 };

	private:
		fig::observer_ptr<VerticalScrollBar> _pScrollBar;
	};
}
