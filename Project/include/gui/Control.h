#pragma once

#include "Types.h"
#include "LayoutElement.h"

class CustomRenderer;

class Control : public LayoutElement
{
public:
	Control(Control* pParent);
	virtual ~Control();

	virtual void Render(SDL_Renderer* pRenderer);
	virtual void Update(float fDeltaTime) override;

	SDL_Color GetForegroundColor() const;
	SDL_Color GetBackgroundColor() const;
	bool GetClipping() const { return _bClipping; }
	virtual void SetForegroundColor(SDL_Color color) { _foregroundColor = color; }
	virtual void SetBackgroundColor(SDL_Color color) { _backgroundColor = color; }
	void SetBorderColor(SDL_Color color) { _borderColor = color; }
	void SetClipping(bool bEnable) { _bClipping = bEnable; }

	bool GetVisible() { return _bVisible; }
	void SetVisible(bool bVisible) { _bVisible = bVisible; }

	bool ProcessEvent(SDL_Event* event);

	void SetBackgroundRenderer(CustomRenderer* pCustom);
	void SetBorderRenderer(CustomRenderer* pCustom);

protected:
	virtual void OnRender(SDL_Renderer* pRenderer);
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
	
	// Theming
	CustomRenderer* _pBGRenderer = nullptr;
	CustomRenderer* _pBorderRenderer = nullptr;
};