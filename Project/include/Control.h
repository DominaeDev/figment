#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include "LayoutElement.h"

class Control : public LayoutElement
{
public:
	Control(Control* pParent);

	virtual void Render(SDL_Renderer* pRenderer);
	virtual void Update(float fDeltaTime) override;

	SDL_Color GetForegroundColor() const;
	SDL_Color GetBackgroundColor() const;
	bool GetClipping() const { return _bClipping; }
	void SetForegroundColor(SDL_Color color) { _foregroundColor = color; }
	void SetBackgroundColor(SDL_Color color) { _backgroundColor = color; }
	void SetBorderColor(SDL_Color color) { _borderColor = color; }
	void SetClipping(bool bEnable) { _bClipping = bEnable; }

	bool GetVisible() { return _bVisible; }
	void SetVisible(bool bVisible) { _bVisible = bVisible; }

	bool ProcessEvent(SDL_Event* event);

protected:
	virtual void OnRender(SDL_Renderer* pRenderer) = 0;
	virtual void OnParent();
	virtual bool OnEvent(SDL_Event* event) { return false; }

	void DrawBackground(SDL_Renderer* pRenderer);
	void DrawBorder(SDL_Renderer* pRenderer);

protected:
	SDL_Color _foregroundColor {};
	SDL_Color _backgroundColor {};
	SDL_Color _borderColor {};
	bool _bClipping = false;
	bool _bVisible = true;
};