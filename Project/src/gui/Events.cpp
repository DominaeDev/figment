#include <pch.h>
#include "gui/Events.h"
#include <cassert>

namespace fig::gui
{
	uint32_t UserEventBase = SDL_EVENT_USER;

	void RegisterUserEvents()
	{
		UserEventBase = SDL_RegisterEvents(static_cast<int32_t>(UserEvent::Count));
		assert(UserEventBase);
	}

}