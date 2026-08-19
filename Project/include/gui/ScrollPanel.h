#pragma once

#include "Control.h"

namespace fig::gui
{
	class VerticalScrollBar;

	class ScrollPanel : public Control
	{
	public:
		ScrollPanel(ControlPtr pParent, bool bScrollBar = true);
		~ScrollPanel();
		void Render(fig::renderer_ptr pRenderer) override;
		inline void SetScrollBarOffset(fig::coord offset) noexcept { _scrollBarOffset = offset; }
		
		void ScrollTo(float position, bool bSmooth = true) noexcept;

		void SetTopPadding(fig::coord padding) { _topPadding = padding; };
		void SetBottomPadding(fig::coord padding) { _bottomPadding = padding; };

		fig::coord GetScrollY() const noexcept { return _currentScrollY; }

	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(fig::event& event) override;
		void OnAfterLayout() override;
		void OnSize() override;

		virtual fig::coord GetExtent() const;
		
		virtual void OnScroll() {};
		void ResetScroll() noexcept;

		bool HandleMouseWheel(SDL_MouseWheelEvent event);
		void RefreshScrollBar();

	protected:
		fig::coord _topPadding {};
		fig::coord _bottomPadding {};
		float _fScrollY {};
		fig::coord _currentScrollY; 
		float _fTargetScrollY {};
		fig::coord _scrollBarOffset { -16 };

	private:
		fig::coord _maxExtent {};
		fig::observer_ptr<VerticalScrollBar> _pScrollBar;
	};
}
