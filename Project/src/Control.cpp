#include "Control.h"
#include "Sizer.h"
#include "Utility.h"

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
		auto renderable = reinterpret_cast<Control*>(child);
		if (renderable)
			renderable->Render(pRenderer);
	}

	if (_bClipping)
	{
		s_pClippingRect = lastClippingRect;
		SDL_SetRenderClipRect(pRenderer, s_pClippingRect);
	}
}

SDL_Color Control::GetForegroundColor() const
{
	if (!ColorIsDefined(_foregroundColor))
	{
		auto frameParent = dynamic_cast<Control*>(_pParent);
		return frameParent ? frameParent->GetForegroundColor() : SDL_Color();
	}
	return _foregroundColor;
}

SDL_Color Control::GetBackgroundColor() const
{
	if (!ColorIsDefined(_backgroundColor))
	{
		auto frameParent = dynamic_cast<Control*>(_pParent);
		return frameParent ? frameParent->GetBackgroundColor() : SDL_Color();
	}
	return _backgroundColor;
}

void Control::ClearBackground(SDL_Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	if (ColorIsDefined(bgColor))
	{
		SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(pRenderer, &_rect);
	}
}
