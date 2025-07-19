#include "gui/Control.h"
#include "gui/Sizer.h"
#include "gui/Color.h"
#include "gui/CustomRenderer.h"

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

void Control::Render(SDL_Renderer* pRenderer)
{
	if (!_bVisible)
		return;

	static SDL_Rect* s_pClippingRect = nullptr;
	SDL_Rect* lastClippingRect = s_pClippingRect;
	SDL_Rect clippingRect;
	if (_bClipping)
	{
		SDL_Rect rect { (int)_rect.x, (int)_rect.y, (int)_rect.w, (int)_rect.h };
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
			renderable->Render(pRenderer);
	}

	if (_bClipping)
	{
		s_pClippingRect = lastClippingRect;
		SDL_SetRenderClipRect(pRenderer, s_pClippingRect);
	}
}

void Control::OnRender(SDL_Renderer* pRenderer)
{
	DrawBackground(pRenderer);
	DrawBorder(pRenderer);
}

void Control::DrawBorder(SDL_Renderer* pRenderer)
{
	// Custom renderer
	if (_pBorderRenderer)
	{
		_pBorderRenderer->Draw(pRenderer, _rect);
		return;
	}

	if (!Color::IsDefined(_borderColor))
		return;
	
	SDL_SetRenderDrawColor(pRenderer, _borderColor.r, _borderColor.g, _borderColor.b, _borderColor.a);
	SDL_RenderRect(pRenderer, &_rect);
}

SDL_Color Control::GetForegroundColor() const
{
	if (!Color::IsDefined(_foregroundColor))
	{
		auto frameParent = dynamic_cast<Control*>(_pParent);
		return frameParent ? frameParent->GetForegroundColor() : SDL_Color();
	}
	return _foregroundColor;
}

SDL_Color Control::GetBackgroundColor() const
{
	if (!Color::IsDefined(_backgroundColor))
	{
		auto frameParent = dynamic_cast<Control*>(_pParent);
		return frameParent ? frameParent->GetBackgroundColor() : SDL_Color();
	}
	return _backgroundColor;
}

void Control::DrawBackground(SDL_Renderer* pRenderer)
{
	// Custom renderer
	if (_pBGRenderer)
	{
		_pBGRenderer->Draw(pRenderer, _rect);
		return;
	}

	auto bgColor = GetBackgroundColor();
	if (Color::IsDefined(bgColor) && bgColor.a != 0)
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
		if (!Color::IsDefined(_foregroundColor))
			_foregroundColor = pParent->GetForegroundColor();
		if (!Color::IsDefined(_backgroundColor))
			_backgroundColor = pParent->GetBackgroundColor();
	}
}

bool Control::ProcessEvent(SDL_Event* event)
{
	if (OnEvent(event))
		return true;

	for (auto it = std::begin(_children); it != std::end(_children); ++it)
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
