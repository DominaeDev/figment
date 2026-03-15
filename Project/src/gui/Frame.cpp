#include <pch.h>
#include "gui/Frame.h"
#include "gui/GUITypes.h"
#include "gui/Window.h"

using namespace fig::gui;

Frame::Frame(Window* pHostWindow) : Control(nullptr, pHostWindow)
{
	int w, h;
	SDL_GetWindowSizeInPixels(pHostWindow->GetSDLWindow().get(), &w, &h);
	SetSize(w, h);
}

void Frame::Render(Renderer* pRenderer)
{
	SDL_SetRenderDrawColor(pRenderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(pRenderer);

	Control::Render(pRenderer);

	SDL_RenderPresent(pRenderer);
}

