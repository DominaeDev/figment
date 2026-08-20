#pragma once

#include "GUITypes.h"

namespace fig::gui
{
	class Frame;
}

template <typename T>
concept DerivesFromFrame = std::derived_from<T, fig::gui::Frame>;

namespace fig::gui
{
	class Window
	{
	public:
		Window(fig::string_view title, int32_t width, int32_t height);
		~Window();

		SDL_WindowID GetWindowID() const;

		void Render();
		void Update(float fElapsed);
		void SetTitle(fig::string_view title);
		
		template <typename T>
		void CreateFrame()
		{
			_pFrame = std::make_unique<T>(this);
		}

		fig::sdl::Window& GetSDLWindow() noexcept { return _window; }
		fig::sdl::Renderer& GetSDLRenderer() noexcept { return _renderer; }
		fig::sdl::TextEngine& GetSDLTextEngine() noexcept { return _textEngine; }

		fig::observer_ptr<Frame> GetFrame() { return _pFrame ? _pFrame.get() : nullptr; }
		
		bool HandleEvent(fig::event& event);

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
