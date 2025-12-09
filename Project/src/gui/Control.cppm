module;

#include <SDL3/SDL.h>

export module GUI.Control;

import Common;
export import GUI.Layout.LayoutElement;
export import GUI.GraphicTypes;
export import CustomRenderer;

export import GUI.Layout;

export
{
	class Control : public LayoutElement
	{
	public:
		Control(Control* pParent);
		virtual ~Control();

		virtual void Render(Renderer* pRenderer);
		virtual void Update(float fDeltaTime) override;

		Color GetForegroundColor() const;
		Color GetBackgroundColor() const;
		bool GetClipping() const { return _bClipping; }
		virtual void SetForegroundColor(Color color) { _foregroundColor = color; }
		virtual void SetBackgroundColor(Color color) { _backgroundColor = color; }
		void SetBorderColor(Color color) { _borderColor = color; }
		void EnableClipping(bool bEnable) { _bClipping = bEnable; }
		void EnableCulling(bool bEnable) { _bCulling = bEnable; }

		bool GetVisible() { return _bVisible; }
		void SetVisible(bool bVisible) { _bVisible = bVisible; }

		bool ProcessEvent(SDL_Event* event);

		void SetBackgroundRenderer(CustomRenderer* pCustom);
		void SetBorderRenderer(CustomRenderer* pCustom);

	protected:
		virtual void OnRender(Renderer* pRenderer);
		virtual void OnParent();
		virtual bool OnEvent(SDL_Event* event) { return false; }

		void DrawBackground(Renderer* pRenderer);
		void DrawBorder(Renderer* pRenderer);

	protected:
		Color _foregroundColor {};
		Color _backgroundColor {};
		Color _borderColor {};
		bool _bClipping = false;
		bool _bCulling = false;
		bool _bVisible = true;

		// Theming
		CustomRenderer* _pBGRenderer = nullptr;
		CustomRenderer* _pBorderRenderer = nullptr;
	};

	class ControlWithMargins : public Control
	{
	public:
		ControlWithMargins(Control* pParent);

		void SetMargins(int left, int top, int right, int bottom);
		void SetMargins(Rect rect);

	protected:
		int _marginLeft = 0;
		int _marginTop = 0;
		int _marginRight = 0;
		int _marginBottom = 0;

		int HMargin() const { return _marginLeft + _marginRight; }
		int VMargin() const { return _marginTop + _marginBottom; }
	};
}

Control::Control(Control* pParent)
{
	if (pParent)
		pParent->AddChild(this);
	_pParent = pParent;
}

Control::~Control()
{
	delete _pBGRenderer;
}

void Control::Update(float fDeltaTime)
{
	if (_bInvalidLayout)
		Layout();

	// Update this
	OnUpdate(fDeltaTime);

	// Update children
	for (auto& child : _children)
		child->Update(fDeltaTime);
}

void Control::Render(Renderer* pRenderer)
{
	if (!_bVisible)
		return;

	static Rect* s_pClippingRect = nullptr;
	Rect* lastClippingRect = s_pClippingRect;
	Rect clippingRect;
	Rect cullingRect = gui_util::expand_rect(gui_util::to_rect(GetRect()), 100);

	if (_bClipping)
	{
		Rect rect { (int)_rect.x, (int)_rect.y, (int)_rect.w, (int)_rect.h };
		if (s_pClippingRect)
			SDL_GetRectIntersection(s_pClippingRect, &rect, &clippingRect);
		else
			clippingRect = rect;

		SDL_SetRenderClipRect(pRenderer, &clippingRect);
		s_pClippingRect = &clippingRect;
	}

	// Draw this
	OnRender(pRenderer);

	// Draw children
	for (auto& child : _children)
	{
		auto renderable = dynamic_cast<Control*>(child);
		if (renderable)
		{
			if (_bCulling)
			{
				auto childRect = gui_util::to_rect(renderable->GetRect());
				if (!SDL_HasRectIntersection(&cullingRect, &childRect))
					continue;
			}

			renderable->Render(pRenderer);
		}
	}

	if (_bClipping)
	{
		s_pClippingRect = lastClippingRect;
		SDL_SetRenderClipRect(pRenderer, s_pClippingRect);
	}
}

void Control::OnRender(Renderer* pRenderer)
{
	DrawBackground(pRenderer);
	DrawBorder(pRenderer);
}

void Control::DrawBorder(Renderer* pRenderer)
{
	// Custom renderer
	if (_pBorderRenderer)
	{
		_pBorderRenderer->Render(pRenderer, _rect);
		return;
	}

	if (!color_util::is_defined(_borderColor))
		return;

	SDL_SetRenderDrawColor(pRenderer, _borderColor.r, _borderColor.g, _borderColor.b, _borderColor.a);
	SDL_RenderRect(pRenderer, &_rect);
}

Color Control::GetForegroundColor() const
{
	if (!color_util::is_defined(_foregroundColor))
	{
		auto frameParent = dynamic_cast<Control*>(_pParent);
		return frameParent ? frameParent->GetForegroundColor() : Color();
	}
	return _foregroundColor;
}

Color Control::GetBackgroundColor() const
{
	if (!color_util::is_defined(_backgroundColor))
	{
		auto frameParent = dynamic_cast<Control*>(_pParent);
		return frameParent ? frameParent->GetBackgroundColor() : Color();
	}
	return _backgroundColor;
}

void Control::DrawBackground(Renderer* pRenderer)
{
	// Custom renderer
	if (_pBGRenderer)
	{
		_pBGRenderer->Render(pRenderer, _rect);
		return;
	}

	auto bgColor = GetBackgroundColor();
	if (color_util::is_defined(bgColor) && bgColor.a != 0)
	{
		SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(pRenderer, &_rect);
	}
}

void Control::OnParent()
{
	LayoutElement::OnParent();

	auto pParent = dynamic_cast<Control*>(_pParent);
	if (pParent)
	{
		if (!color_util::is_defined(_foregroundColor))
			_foregroundColor = pParent->GetForegroundColor();
		if (!color_util::is_defined(_backgroundColor))
			_backgroundColor = pParent->GetBackgroundColor();
	}
}

bool Control::ProcessEvent(SDL_Event* event)
{
	if (OnEvent(event))
		return true;

	for (auto it = _children.begin(); it != std::end(_children); ++it)
	{
		Control* pControl = dynamic_cast<Control*>(*it);
		if (pControl && pControl->ProcessEvent(event))
			return true;
	}
	return false;
}

void Control::SetBackgroundRenderer(CustomRenderer* pCustom)
{
	if (_pBGRenderer != nullptr)
	{
		delete _pBGRenderer;
		_pBGRenderer = nullptr;
	}
	_pBGRenderer = pCustom;
}

void Control::SetBorderRenderer(CustomRenderer* pCustom)
{
	if (_pBorderRenderer != nullptr)
	{
		delete _pBorderRenderer;
		_pBorderRenderer = nullptr;
	}
	_pBorderRenderer = pCustom;
}

ControlWithMargins::ControlWithMargins(Control* pParent) : Control(pParent)
{
}

void ControlWithMargins::SetMargins(int left, int top, int right, int bottom)
{
	_marginLeft = left;
	_marginTop = top;
	_marginRight = right;
	_marginBottom = bottom;
}

void ControlWithMargins::SetMargins(Rect rect)
{
	SetMargins(rect.x, rect.y, rect.w, rect.h);
}