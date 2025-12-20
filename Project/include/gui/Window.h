#ifndef WINDOW_H__
#define WINDOW_H__
#pragma once

#include "GUITypes.h"
#include "IUpdateable.h"

namespace fig::gui
{
	class Frame;
}

template <typename T>
concept DerivesFromFrame = std::derived_from<T, fig::gui::Frame>;

namespace fig::gui
{
	class Window : IUpdateable
	{
	public:
		Window(fig::string_view title, int32_t width, int32_t height);
		~Window();

		SDL_WindowID GetWindowID() const;

		void Render();
		void Update(float fDeltaTime);
		void SetTitle(fig::string_view title);
		
		template <typename T>
		void CreateFrame()
		{
			_pFrame = std::make_unique<T>(this);
		}

		fig::sdl::Window& GetSDLWindow() noexcept { return _window; }
		fig::sdl::Renderer& GetSDLRenderer() noexcept { return _renderer; }
		fig::sdl::TextEngine& GetSDLTextEngine() noexcept { return _textEngine; }

		std::weak_ptr<Frame> GetFrame() { return _pFrame; }
		
		bool HandleEvent(fig::gui::Event& event);

	protected:
		bool OnKeyboardEvent(SDL_KeyboardEvent& event);

	private:
		fig::sdl::Window _window;
		fig::sdl::Renderer _renderer;
		fig::sdl::TextEngine _textEngine;
		std::shared_ptr<Frame> _pFrame {};

		int32_t _width;
		int32_t _height;
	};
}
#endif