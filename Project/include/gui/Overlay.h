#pragma once

#include "gui/Control.h"

namespace fig::gui
{
	class Overlay : public Control
	{
		friend class Frame;
	public:
		Overlay(Frame* pHostFrame);

	protected:
		fig::observer_ptr<Frame> _pOwner;
		bool _bDestroyMe = false;
	};
}
