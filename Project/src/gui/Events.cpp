#include <pch.h>
#include "gui/Events.h"

namespace fig::gui
{
	uint32_t UserEventBase = SDL_EVENT_USER;

	void RegisterUserEvents()
	{
		UserEventBase = SDL_RegisterEvents(EventType::Count);
	}

}