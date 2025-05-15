#include "Frame.h"
#include <SDL3/SDL.h>

Frame::Frame(SDL_Window* pWindow)
{
	_pWindow = pWindow;
	if (pWindow)
	{
		int w, h;
		SDL_GetWindowSizeInPixels(pWindow, &w, &h);
		SetSize((float)w, (float)h);
	}
}

Frame::~Frame()
{
}

SDL_WindowID Frame::GetWindowID() const
{
	return SDL_GetWindowID(_pWindow);
}

bool Frame::OnEvent(SDL_Event* event)
{
	if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
	{
		if (GetWindowID() == event->window.windowID)
		{
			SetSize((float)event->window.data1, (float)event->window.data2);
			return true;
		}
	}

	return false;
}
