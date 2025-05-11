#include "Frame.h"
#include "Sizer.h"
#include "Utility.h"

Frame::~Frame()
{
	for (auto& child : _children)
		delete child;

	delete _pSizer;
}

void Frame::Update(float fDeltaTime)
{
	if (_bInvalidLayout)
		Layout();

	// Update this
	OnUpdate(fDeltaTime);

	// Update children
	for (auto& child : _children)
		child->Update(fDeltaTime);
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

void Frame::Layout()
{
	if (_pSizer)
		_pSizer->Layout(_rect);
	else
	{
		// Update child positions
		for (auto& child : _children)
		{
			auto& rect = child->GetRect();
			auto position = child->GetAbsolutePosition();
			rect.x = position.x;
			rect.y = position.y;
		}
	}
	_bInvalidLayout = false;
}

SDL_Color Frame::GetForegroundColor() const
{
	if (!ColorIsDefined(_foregroundColor))
		return _pParent ? _pParent->GetForegroundColor() : SDL_Color();
	return _foregroundColor;
}

SDL_Color Frame::GetBackgroundColor() const
{
	if (!ColorIsDefined(_backgroundColor))
		return _pParent ? _pParent->GetBackgroundColor() : SDL_Color();
	return _backgroundColor;

}

void Frame::ClearBackground(SDL_Renderer* pRenderer)
{
	auto bgColor = GetBackgroundColor();
	if (ColorIsDefined(bgColor))
	{
		SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(pRenderer, &_rect);
	}
}

void Frame::SetRect(SDL_FRect rect)
{
	SetPosition(rect.x, rect.y);
	SetSize(rect.w, rect.h);
	_bInvalidLayout = true;
	OnSize();
}

void Frame::SetRect(float x, float y, float width, float height)
{
	SetPosition(x, y);
	SetSize(width, height);
	_bInvalidLayout = true;
	OnSize();
}

SDL_FPoint Frame::GetAbsolutePosition() const
{
	if (_pParent)
	{
		auto position = SDL_FPoint { _pParent->_rect.x, _pParent->_rect.y };
		position.x += _position.x;
		position.y += _position.y;
		return position;
	}

	return _position;
}

void Frame::SetPosition(float x, float y)
{
	SetPosition(SDL_FPoint(x, y));
}

void Frame::SetPosition(SDL_FPoint position)
{
	_position = position;
	position = GetAbsolutePosition();
	_rect = SDL_FRect(position.x, position.y, _rect.w, _rect.h);
	_bInvalidLayout = true;
	OnSize();
}

void Frame::SetSize(SDL_FPoint size)
{
	_size = size;
	_rect.w = size.x;
	_rect.h = size.y;
	_bInvalidLayout = true;
	OnSize();
}

void Frame::SetSize(float width, float height)
{
	SetSize(SDL_FPoint(width, height));
}

void Frame::SetWidth(float width)
{
	SetSize(SDL_FPoint(width, _size.y));
}

void Frame::SetHeight(float height)
{
	SetSize(SDL_FPoint(_size.x, height));
}

void Frame::AddChild(Frame* pFrame)
{
	pFrame->SetParent(this);
	_children.push_back(pFrame);
}

bool Frame::RemoveChild(Frame* frame)
{
	auto it = std::find(std::begin(_children), std::end(_children), frame);
	if (it != std::end(_children))
	{
		(*it)->SetParent(nullptr);
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

void Frame::SetParent(Frame* pParent)
{
	_pParent = pParent;
	SetPosition(_position);
}