#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include "IUpdateable.h"

class Sizer;

class LayoutElement : public IUpdateable
{
public:
	virtual ~LayoutElement();

	virtual void Update(float fDeltaTime) = 0;

	SDL_FRect& GetRect() { return _rect; }
	SDL_FPoint GetPosition() const { return _position; }
	SDL_FPoint GetSize() const { return _size; }

	SDL_FPoint GetAbsolutePosition() const;
	float GetLeft() const { return _position.x; }
	float GetTop() const { return _position.y; }
	float GetWidth() const { return _size.x; }
	float GetHeight() const { return _size.y; }
	const SDL_FPoint& GetMinSize() const { return _minSize; }
	const SDL_FPoint& GetMaxSize() const { return _maxSize; }

	void SetRect(SDL_FRect rect);
	void SetRect(float x, float y, float width, float height);
	void SetPosition(SDL_FPoint position);
	void SetPosition(float x, float y);
	void SetSize(SDL_FPoint size);
	void SetSize(float width, float height);
	void SetWidth(float width);
	void SetHeight(float height);

	void SetMinSize(SDL_FPoint size) { _minSize = size; }
	void SetMinSize(float width, float height) { _minSize = SDL_FPoint { width, height }; }
	void SetMaxSize(SDL_FPoint size) { _maxSize = size; }
	void SetMaxSize(float width, float height) { _maxSize = SDL_FPoint { width, height }; }
	
	void AddChild(LayoutElement* pFrame);
	bool RemoveChild(LayoutElement* pFrame);

	void SetSizer(Sizer* sizer);
	void InvalidateLayout() { _bInvalidLayout = true; }

protected:
	void SetParent(LayoutElement* pParent);
	void Layout();

	virtual void OnSize();

protected:
	std::vector<LayoutElement*> _children;
	LayoutElement* _pParent = nullptr;
	Sizer* _pSizer = nullptr;

	SDL_FRect _rect = {};
	SDL_FPoint _position = {};
	SDL_FPoint _size = {};
	SDL_FPoint _minSize = {};
	SDL_FPoint _maxSize = {};
	bool _bInvalidLayout = false;
};