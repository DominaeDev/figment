#include "Frame.h"

Frame::Frame()
{
}

Frame::~Frame()
{
	for (auto& child : _children)
		delete child;
}

void Frame::Update(float fDeltaTime)
{
	OnUpdate(fDeltaTime);
	for (auto& child : _children)
		child->OnUpdate(fDeltaTime);
}

void Frame::Render(SDL_Renderer* pRenderer)
{
	OnRender(pRenderer);
	for (auto& child : _children)
		child->OnRender(pRenderer);
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
	OnSize();
}

void Frame::SetRect(float x, float y, float width, float height)
{
	_rect = SDL_FRect { x, y, width, height };
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
	OnSize();
}

void Frame::SetSize(float width, float height)
{
	_rect.w = width;
	_rect.h = height;
	OnSize();
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