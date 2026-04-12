#include <pch.h>
#include "util/Events.h"

namespace fig::gui
{
	uint32_t USER_EVENT_BASE = SDL_EVENT_USER;

	void RegisterUserEvents()
	{
		USER_EVENT_BASE = SDL_RegisterEvents(EventType::Count);
	}

}