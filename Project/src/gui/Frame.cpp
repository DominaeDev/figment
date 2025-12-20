#include "gui/Frame.h"
#include "gui/GUITypes.h"
#include "gui/Window.h"

using namespace fig::gui;

Frame::Frame(Window* pHostWindow) : Control(nullptr, pHostWindow)
{
	int w, h;
	SDL_GetWindowSizeInPixels(pHostWindow->GetSDLWindow(), &w, &h);
	SetSize(toF(w), toF(h));
}

Frame::~Frame()
{
}

void Frame::Render(Renderer* pRenderer)
{
	SDL_SetRenderDrawColor(pRenderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(pRenderer);

	Control::Render(pRenderer);

	SDL_RenderPresent(pRenderer);
}

bool Frame::OnEvent(Event& event)
{
	if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
	{
		if (OnKeyboardEvent(event.key))
			return true;
	}

	return false;
}
