#pragma once

#include <SDL3/SDL.h>
#include <vector>

class Sizer;

class Frame
{
public:
	Frame();
	virtual ~Frame();

	void Update(float fDeltaTime);
	void Render(SDL_Renderer* pRenderer);

	SDL_Color GetForegroundColor() const { return _foregroundColor; }
	SDL_Color GetBackgroundColor() const { return _backgroundColor; }
	SDL_FRect& GetRect() { return _rect; }
	SDL_FPoint GetPosition() const { return SDL_FPoint { _rect.x, _rect.y }; }
	SDL_FPoint GetSize() const { return SDL_FPoint { _rect.w, _rect.h }; }
	const SDL_FPoint& GetPreferredSize() const { return _preferredSize; }
	float GetWidth() const { return _rect.w; }
	float GetHeight() const { return _rect.h; }
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
	void SetPreferredSize(SDL_FPoint size);
	void SetPreferredSize(float width, float height);
	void SetMinSize(SDL_FPoint size) { _minSize = size; }
	void SetMinSize(float width, float height) { _minSize = SDL_FPoint { width, height }; }
	void SetMaxSize(SDL_FPoint size) { _maxSize = size; }
	void SetMaxSize(float width, float height) { _maxSize = SDL_FPoint { width, height }; }

	void SetClipping(bool bEnable) { _bClipping = bEnable; }

	void AddChild(Frame* pFrame);
	bool RemoveChild(Frame* pFrame);

	void SetSizer(Sizer* sizer);
	void InvalidateLayout();

protected:
	void ClearBackground(SDL_Renderer* pRenderer);

	virtual void OnUpdate(float fDeltaTime) {};
	virtual void OnRender(SDL_Renderer* pRenderer) {};
	virtual void OnSize() {}

protected:
	SDL_Color _foregroundColor;
	SDL_Color _backgroundColor;
	SDL_FRect _rect = {};
	SDL_FPoint _preferredSize = {};
	SDL_FPoint _minSize = {};
	SDL_FPoint _maxSize = {};

	std::vector<Frame*> _children;
	Sizer* _pSizer = nullptr;
	bool _bInvalidLayout = false;
	bool _bClipping = true;
};