#ifndef OVERLAY_H__
#define OVERLAY_H__
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
		Frame* _pOwner = nullptr;
		bool _bDestroyMe = false;
	};
}

#endif