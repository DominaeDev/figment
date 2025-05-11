#pragma once

#include <SDL3/SDL.h>
#include <vector>

class Sizer;

class Frame
{
public:
	virtual ~Frame();

	void Update(float fDeltaTime);
	void Render(SDL_Renderer* pRenderer);

	SDL_Color GetForegroundColor() const;
	SDL_Color GetBackgroundColor() const;
	SDL_FRect& GetRect() { return _rect; }

	SDL_FPoint GetAbsolutePosition() const;
	SDL_FPoint GetPosition() const { return _position; }
	SDL_FPoint GetSize() const { return _size; }

	float GetLeft() const { return _position.x; }
	float GetTop() const { return _position.y; }
	float GetWidth() const { return _size.x; }
	float GetHeight() const { return _size.y; }
	const SDL_FPoint& GetMinSize() const { return _minSize; }
	const SDL_FPoint& GetMaxSize() const { return _maxSize; }

	void SetForegroundColor(SDL_Color color) { _foregroundColor = color; }
	void SetBackgroundColor(SDL_Color color) { _backgroundColor = color; }

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

	void SetClipping(bool bEnable) { _bClipping = bEnable; }

	void AddChild(Frame* pFrame);
	bool RemoveChild(Frame* pFrame);

	void SetSizer(Sizer* sizer);
	void InvalidateLayout() { _bInvalidLayout = true; }

protected:
	void ClearBackground(SDL_Renderer* pRenderer);
	void SetParent(Frame* pParent);
	void Layout();

	virtual void OnUpdate(float fDeltaTime) {};
	virtual void OnRender(SDL_Renderer* pRenderer) {};
	virtual void OnSize() {}

protected:
	SDL_Color _foregroundColor;
	SDL_Color _backgroundColor;
	SDL_FRect _rect = {};

	SDL_FPoint _position = {};
	SDL_FPoint _size = {};
	SDL_FPoint _minSize = {};
	SDL_FPoint _maxSize = {};

	std::vector<Frame*> _children;
	Frame* _pParent = nullptr;
	Sizer* _pSizer = nullptr;
	bool _bInvalidLayout = false;
	bool _bClipping = true;
};