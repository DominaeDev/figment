#ifndef SCROLLBAR_H__
#define SCROLLBAR_H__
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

		void SetScroll(float fPosition, Coord maxExtent);

	protected:
		void OnUpdate(float fElapsed) override;
		bool OnEvent(Event& event) override;

	private:
		void RefreshHandle();

	protected:
		TexturedBorder* _pHandle {};
		float _fPosition {};
		float _fExtent {};
		bool _bDirty {};
		bool _bScrolling {};
	};
}

#endif