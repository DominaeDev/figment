#include "Frame.h"
#include <SDL3/SDL.h>

static bool OnEvent(void* data, SDL_Event* event)
{
	if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
	{
		Frame* pFrame = (Frame*)data;
		if (pFrame->GetWindowID() == event->window.windowID)
			pFrame->SetSize((float)event->window.data1, (float)event->window.data2);
	}
	return true;
}

Frame::Frame(SDL_Window* pWindow)
{
	_pWindow = pWindow;
	if (pWindow)
	{
		int w, h;
		SDL_GetWindowSizeInPixels(pWindow, &w, &h);
		SetSize((float)w, (float)h);
	}

	SDL_AddEventWatch(OnEvent, this);
}

Frame::~Frame()
{
	SDL_RemoveEventWatch(OnEvent, this);
}

SDL_WindowID Frame::GetWindowID() const
{
	return SDL_GetWindowID(_pWindow);
}