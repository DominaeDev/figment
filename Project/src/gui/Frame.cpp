#include <pch.h>
#include "gui/Frame.h"
#include "gui/Overlay.h"
#include "gui/GUITypes.h"
#include "gui/Window.h"

namespace fig::gui
{
	Frame::Frame(Window* pHostWindow) : Control(nullptr, pHostWindow)
	{
		int w, h;
		SDL_GetWindowSizeInPixels(pHostWindow->GetSDLWindow().get(), &w, &h);
		SetSize(w, h);
	}

	Frame::~Frame()
	{
		DestroyOverlays();
	}

	void Frame::Update(float fElapsed)
	{
		Control::Update(fElapsed);

		// Draw overlays
		for (int32_t i = toI(_overlays.size()) - 1; i >= 0; --i)
		{
			auto& pOverlay = _overlays[toUZ(i)];
			pOverlay->Update(fElapsed);

			if (pOverlay->_bDestroyMe)
				DestroyOverlay(pOverlay);
		}
	}

	void Frame::Render(Renderer* pRenderer)
	{
		SDL_SetRenderDrawColor(pRenderer, 255, 0, 255, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(pRenderer);

		Control::Render(pRenderer);

		// Draw overlays
		for (auto it = _overlays.cbegin(); it != _overlays.cend(); ++it)
			(*it)->Render(pRenderer);

		SDL_RenderPresent(pRenderer);
	}

	void Frame::AddOverlay(Overlay* pOverlay)
	{
		_overlays.push_back(pOverlay);
	}

	void Frame::DestroyOverlay(Overlay* pOverlay)
	{
		if (auto itFind = std::find(_overlays.begin(), _overlays.end(), pOverlay); itFind != _overlays.end())
		{
			_overlays.erase(itFind);
			delete pOverlay;
		}
	}

	void Frame::DestroyOverlays()
	{
		for (auto pOverlay : _overlays)
			delete pOverlay;
		_overlays.clear();
	}

	bool Frame::ProcessEvent(Event& event)
	{
		for (int32_t i = toI(_overlays.size()) - 1; i >= 0; --i)
		{
			if (_overlays[toUZ(i)]->ProcessEvent(event))
				return true;
		}
		
		return Control::ProcessEvent(event);
	}
}