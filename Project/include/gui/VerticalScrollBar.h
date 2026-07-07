#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class TexturedBorder;
	class VerticalScrollBar : public Control
	{
		friend class ScrollPanel;
	public:
		VerticalScrollBar(LayoutElement* pParent);

		void SetScroll(ScrollPanel& scrollPanel, float fPosition, Coord maxExtent);
	
	protected:
		void OnUpdate(float fElapsed) override;
		EventResult OnEvent(Event& event) override;
		bool HandleMouseDown(int32_t x, int32_t y);
		bool HandleMouseUp(int32_t x, int32_t y);
		bool HandleMouseMotion(int32_t x, int32_t y);

	private:
		void RefreshHandle();

	protected:
		fig::observer_ptr<TexturedBorder> _pHandle;
		fig::observer_ptr<ScrollPanel> _pScrollPanel;
		float _fPosition {};
		float _fExtent {};
		bool _bDirty {};
		bool _bScrolling {};
	};
}