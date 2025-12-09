module;

export module Frame;
export import GUI.Control;

import Common;
export import GUI.GraphicTypes;

export
{
	class Frame : public Control
	{
	public:
		Frame(Window* pWindow);
		virtual ~Frame();

		Window* GetWindow() const { return _pWindow; }
		WindowID GetWindowID() const;

	protected:
		bool OnEvent(SDL_Event* event) override;

		Window* _pWindow = nullptr;
	};
}

Frame::Frame(SDL_Window* pWindow) : Control(nullptr)
{
	_pWindow = pWindow;
	if (pWindow)
	{
		int w, h;
		SDL_GetWindowSizeInPixels(pWindow, &w, &h);
		SetSize(toF(w), toF(h));
	}
}

Frame::~Frame()
{
}

WindowID Frame::GetWindowID() const
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
