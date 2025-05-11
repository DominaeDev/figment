#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include "LayoutElement.h"

class Control : public LayoutElement
{
public:
	virtual void Render(SDL_Renderer* pRenderer);
	virtual void Update(float fDeltaTime) override;

	SDL_Color GetForegroundColor() const;
	SDL_Color GetBackgroundColor() const;
	bool GetClipping() const { return _bClipping; }
	void SetForegroundColor(SDL_Color color) { _foregroundColor = color; }
	void SetBackgroundColor(SDL_Color color) { _backgroundColor = color; }
	void SetClipping(bool bEnable) { _bClipping = bEnable; }

protected:
	virtual void OnUpdate(float fDeltaTime) = 0;
	virtual void OnRender(SDL_Renderer* pRenderer) = 0;

	void ClearBackground(SDL_Renderer* pRenderer);

protected:
	SDL_Color _foregroundColor;
	SDL_Color _backgroundColor;
	bool _bClipping = true;
};