#pragma once

#include "Control.h"
#include "Graphics.h"

class VerticalGradient : public Control
{
public:
	VerticalGradient(Control* pParent, SDL_Color colorTop, SDL_Color colorBottom);
	void SetColors(SDL_Color colorTop, SDL_Color colorBottom);

protected:
	void OnUpdate(float fDeltaTime) override {};
	void OnRender(SDL_Renderer* pRenderer) override;

	void RefreshGeometry(SDL_FRect rect);
private:
	SDL_FColor _colorTop {};
	SDL_FColor _colorBottom {};
	SDL_FRect _lastRect {};
	SDL_Texture* _pTexture {};

	std::vector<SDL_Vertex> _vertices {};
};