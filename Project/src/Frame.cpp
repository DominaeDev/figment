#include "Frame.h"
#include "Sizer.h"

Frame::Frame()
{
}

Frame::~Frame()
{
	for (auto& child : _children)
		delete child;

	delete _pSizer;
}

void Frame::Update(float fDeltaTime)
{
	// Layout
	if (_pSizer && _bInvalidLayout)
	{
		_pSizer->Layout(_rect);
		_bInvalidLayout = false;
	}

	// Update this
	OnUpdate(fDeltaTime);

	// Update children
	for (auto& child : _children)
		child->OnUpdate(fDeltaTime);
}


void Frame::Render(SDL_Renderer* pRenderer)
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
		child->Render(pRenderer);

	if (_bClipping)
	{
		s_pClippingRect = lastClippingRect;
		SDL_SetRenderClipRect(pRenderer, s_pClippingRect);
	}
}

void Frame::ClearBackground(SDL_Renderer* pRenderer)
{
	if (_backgroundColor.a == 0)
		return;

	SDL_SetRenderDrawColor(pRenderer, _backgroundColor.r, _backgroundColor.g, _backgroundColor.b, _backgroundColor.a);
	SDL_RenderFillRect(pRenderer, &_rect);
}

void Frame::SetRect(SDL_FRect rect)
{
	_rect = rect;
	_bInvalidLayout = true;
	OnSize();
}

void Frame::SetRect(float x, float y, float width, float height)
{
	_rect = SDL_FRect { x, y, width, height };
	_bInvalidLayout = true;
	OnSize();
}

void Frame::SetPosition(SDL_FPoint position)
{
	_rect.x = position.x;
	_rect.y = position.y;
	OnSize();
}

void Frame::SetPosition(float x, float y)
{
	_rect.x = x;
	_rect.y = y;
	OnSize();
}

void Frame::SetSize(SDL_FPoint size)
{
	_rect.w = size.x;
	_rect.h = size.y;
	_bInvalidLayout = true;
	OnSize();
}

void Frame::SetSize(float width, float height)
{
	_rect.w = width;
	_rect.h = height;
	_bInvalidLayout = true;
	OnSize();
}

void Frame::SetPreferredSize(SDL_FPoint size)
{
	_preferredSize = size;
}

void Frame::SetPreferredSize(float width, float height)
{
	_preferredSize.x = width;
	_preferredSize.y = height;
}

void Frame::AddChild(Frame* frame)
{
	_children.push_back(frame);
}

bool Frame::RemoveChild(Frame* frame)
{
	auto it = std::find(std::begin(_children), std::end(_children), frame);
	if (it != std::end(_children))
	{
		_children.erase(it);
		return true;
	}
	return false;
}

void Frame::SetSizer(Sizer* pSizer)
{
	delete _pSizer;
	_pSizer = pSizer;
	_bInvalidLayout = true;
}