#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	using FramePtr = fig::observer_ptr<class Frame>;

	class Overlay : public Control
	{
		friend class Frame;
	public:
		Overlay(FramePtr pHostFrame);
		virtual ~Overlay() {};

	protected:
		fig::observer_ptr<Frame> _pOwner;

		void Destroy();

	private:
		bool _bDestroyMe = false;
	};
}
