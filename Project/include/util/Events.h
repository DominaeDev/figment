#ifndef EVENTS_H__
#define EVENTS_H__
#pragma once

#include <stdint.h>

namespace fig::gui
{
	enum EventType : uint32_t
	{
		MenuOpened,
		MenuClosed,

		Count,
	};
	
	extern void RegisterUserEvents();
	extern uint32_t USER_EVENT_BASE;

#define USER_EVENT(X) (USER_EVENT_BASE + X)
}
#endif
