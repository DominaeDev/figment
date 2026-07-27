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

	bool IsBroadcastEvent(fig::event& event)
	{
		if (not IsUserEvent(event))
			return false;
		if (event.type >= SDLUserEvent(UserEvent::LLMStatusUpdate))
			return true;
		return false;
	}
}