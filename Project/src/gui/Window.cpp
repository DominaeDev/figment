#include <pch.h>
#include "gui/Window.h"
#include "gui/Frame.h"
#include "model/GlobalStrings.h"
#include "Constants.h"
#include "gui/AppResources.h"
#include "gui/OldCharacterImageStore.h"

using namespace fig::gui;

Window::Window(fig::string_view title, int32_t width, int32_t height) :
	_width { width },
	_height { height }
{
	_window = fig::sdl::Window(toCStr(title), width, height, SDL_WINDOW_RESIZABLE);
	if (!_window)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		throw std::runtime_error("Couldn't create window");
	}

	_renderer = fig::sdl::Renderer(_window.get(), nullptr);
	if (!_renderer)
	{
		SDL_Log("Couldn't create renderer: %s", SDL_GetError());
		throw std::runtime_error("Couldn't create renderer");
	}

	_textEngine = fig::sdl::TextEngine(_renderer.get());
	if (!_textEngine)
	{
		SDL_Log("Couldn't create text engine: %s", SDL_GetError());
		throw std::runtime_error("Couldn't create text engine");
	}

	SDL_SetWindowMinimumSize(_window.get(), 800, 400);
	SDL_SetRenderVSync(_renderer.get(), 1);

	// Load textures
	AppResources::Init(_renderer.get()); //! @temp

	// Load character images
	OldCharacterImageStore::Init(_renderer.get());	//! @temp
}

Window::~Window()
{
	OldCharacterImageStore::Release();
	AppResources::Release();
}

SDL_WindowID Window::GetWindowID() const
{
	return SDL_GetWindowID(_window.get());
}

void Window::Update(float fElapsed)
{
	_pFrame->Update(fElapsed);
}

void Window::Render()
{
	_pFrame->Render(_renderer.get());
}

void Window::SetTitle(fig::string_view title)
{
	string str { title };
	SDL_SetWindowTitle(_window.get(), str.c_str());
}

bool Window::HandleEvent(fig::gui::Event& event)
{
	if (!_pFrame)
		return false;
	
	if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
	{
		if (GetWindowID() == event.window.windowID)
		{
			_pFrame->SetSize((float)event.window.data1, (float)event.window.data2);
			return true;
		}
	}
	else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP)
	{
		if (OnKeyboardEvent(event.key))
			return true;
	}

	return _pFrame->ProcessEvent(event);
}

bool Window::OnKeyboardEvent(SDL_KeyboardEvent& event)
{
	bool bShiftDown = (event.mod & SDL_KMOD_SHIFT) != 0;
	bool bAltDown = (event.mod & SDL_KMOD_ALT) != 0;
	bool bCtrlDown = (event.mod & SDL_KMOD_CTRL) != 0;

	if (event.down && !event.repeat)
	{
		switch (event.key)
		{
		case SDLK_RETURN:
			if (bAltDown && !bShiftDown && !bCtrlDown)
			{
				if (SDL_GetWindowFlags(_window.get()) & SDL_WINDOW_MAXIMIZED)
					SDL_RestoreWindow(_window.get());
				else
					SDL_MaximizeWindow(_window.get());
			}
		}
	}

	return false;
}